#include <array>
#include <algorithm>
#include <arpa/inet.h>

#include "ProtocolDNS.hpp"
#include "util.hpp"
#include "Model.hpp"
#include "Snoop.hpp"

//  Values for some selected resource record types.
constexpr int RR_A = 1;
constexpr int RR_PTR = 12;


/*  Parses a sequence of labels into a DNS name appended to :name:.
 *  :label: should point to the first label.
 *  :frame: the DNS datagram which may provide compressed data.
 *  :eof: end of frame, one byte past the end of the datagram.
 *  Returns a pointer to the first byte after the name on success.  Nullptr on error.
 */
const unsigned char* decompress(const unsigned char* label, const unsigned char* frame, const unsigned char* eof, std::string& name, int depth = 0)
{
    if (depth > 10)
        return nullptr;

    for (;;) {
        if (label >= eof)
            return nullptr;
        int label_length = *label;
        int flag = *(label++) >> 6;
        switch (flag) {
            case 0b01: // Illegal.
            case 0b10: // Illegal.
                return nullptr;

            // Uncompressed label.
            case 0b00:
                if (label_length == 0)
                    return label; // Zero-length label marks the end of the name.
                if (label+label_length >= eof)
                    return nullptr; // Error: label spans end of packet.
                if (name.size())
                    name.append(".");
                name.append(reinterpret_cast<const char*>(label), label_length);
                label += label_length;
                break;

            case 0b11: {
                if (label >= eof)
                    return nullptr;
                int pointer = ((label_length & 0x3f) << 8) | *label++;
                if (frame+pointer >= eof)
                    return nullptr;
                if (decompress(frame+pointer, frame, eof, name, depth+1))
                    return label;
                else
                    return nullptr;
            }
        }
    }
}


static bool caseless_compare(const std::string& a, const std::string& b)
{
    return a.size() == a.size() && std::equal(a.begin(), a.end(), a.begin(),
        [](char a, char b){ return std::tolower(a) == std::tolower(b); }
    );
}


/*
 *  Parse a name like "41.2.168.192.in-addr.arpa" and set :address:.
 *  Returns true on success.
 */
static bool parse_ptr_address(const std::string& name, IPV4Address& address)
{
    size_t pos = 0;
    for (int o=3; o>=0; --o) {
        address[o] = 0;
        for (;;) {
            if (pos >= name.size())
                return false;
            else if (isdigit(name[pos]))
                address[o] = address[o] * 10 + (name[pos++]-'0');
            else if (name[pos] == '.') {
                pos++;
                break;
            }
            else
                return false;
        }
    }
    return caseless_compare(name.substr(pos), "in-addr.arpa");
}


/*
From RFC-1035:
    The header contains the following fields:

                                        1  1  1  1  1  1
          0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |                      ID                       |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |                    QDCOUNT                    |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |                    ANCOUNT                    |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |                    NSCOUNT                    |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
        |                    ARCOUNT                    |
        +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
*/


Disposition ProtocolDNS::put(Snoop& snoop, int dir, const unsigned char* payload, int length)
{
    struct dns_header_st {
        uint16_t id;
        uint16_t flags;
        uint16_t qdcount;
        uint16_t ancount;
        uint16_t nscount;
        uint16_t arcount;
    };
    if (unsigned(length) < sizeof(struct dns_header_st))
        return Disposition::TRUNCATED;
    const dns_header_st* header = reinterpret_cast<const dns_header_st*>(payload);

    uint16_t flags = ntohs(header->flags);
    bool response = (flags & 0x8000) == 0x8000;
    if (!response)
        return Disposition::DNS;
    const unsigned char* ptr = payload + sizeof(dns_header_st);
    const unsigned char* end = payload + length;

    //  Read past the question section.
    //  TODO: Can the names in the question section be compressed?
    for (int i=0; i<ntohs(header->qdcount); ++i) {
        if (ptr >= end)
            return Disposition::TRUNCATED;
        int label_length;
        while ((label_length = *ptr++)) {
            //  Max label length is 63 octets.
            if (label_length >= 0xc0)
                return Disposition::DNS_ERROR;
            if (ptr >= end)
                return Disposition::TRUNCATED;
            ptr += label_length;
            if (ptr >= end)
                return Disposition::TRUNCATED;
        }
        ptr += sizeof(uint16_t); //  Skip QTYPE.
        ptr += sizeof(uint16_t); //  Skip QCLASS.
        if (ptr >= end)
            return Disposition::TRUNCATED;
    }

    //  For each of the three remaining sections....
    std::array section_counts { ntohs(header->ancount), ntohs(header->nscount), ntohs(header->arcount) };
    for (unsigned rcount : section_counts) {

        //  For each resource record in this section....
        for (unsigned rr_ix = 0; rr_ix < rcount; ++rr_ix) {

            //  Parse the name field.
            std::string name;
            ptr = decompress(ptr, payload, payload+length, name);
            if (!ptr)
                return Disposition::DNS_ERROR;

            //  TYPE field.
            unsigned type = (*ptr++) << 8;
            if (ptr >= end)
                return Disposition::TRUNCATED;
            type += *ptr++;
            if (ptr >= end)
                return Disposition::TRUNCATED;

            //  CLASS field.
            unsigned rclass = (*ptr++) << 8;
            if (ptr >= end)
                return Disposition::TRUNCATED;
            rclass += *ptr++;
            if (ptr >= end)
                return Disposition::TRUNCATED;

            //  TTL field.
            ptr += 4; //  Skip the four octet TTL field.
            if (ptr >= end)
                return Disposition::TRUNCATED;

            //  RDLENGTH field.
            unsigned short rdlength = (*ptr++) << 8;
            if (ptr >= end)
                return Disposition::TRUNCATED;
            rdlength += *ptr++;
            if (ptr >= end)
                return Disposition::TRUNCATED;

            //  RDATA field.
            const unsigned char* rdata_start = ptr;
            ptr += rdlength;
            if (ptr > end)
                return Disposition::TRUNCATED;

            //  TODO: These names aren't necessarily ASCII 7-bit.
            //        We'll need to do something about that before converting them to UTF-8
            //        in Protobuf messages.

            if (rclass == 1 && type == RR_A) {
                if (rdlength != 4) // A record should have a 4-octet IP address.
                    return Disposition::DNS_ERROR;
                IPV4Address ipaddr;
                std::copy_n(rdata_start, 4, ipaddr.begin());
                snoop.get_model().note_name(ipaddr, name, Model::NameType::DNS);
            }
            if (rclass == 1 && type == RR_PTR) {
                std::string ptr_name;
                if (!decompress(rdata_start, payload, payload+length, ptr_name))
                    return Disposition::DNS_ERROR;
                IPV4Address ipaddr;
                if (parse_ptr_address(name, ipaddr))
                    snoop.get_model().note_name(ipaddr, ptr_name, Model::NameType::DNS);
            }
        }
    }

    return Disposition::DNS;
}
