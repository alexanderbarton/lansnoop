#include <array>
#include <algorithm>
#include <arpa/inet.h>

#include "ProtocolDNS.hpp"
#include "util.hpp"
#include "Model.hpp"
#include "Snoop.hpp"

#include "/home/abarton/debug.hpp"

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

    //  For each of the three remaining three sections....
    std::array section_counts { ntohs(header->ancount), ntohs(header->nscount), ntohs(header->arcount) };
    for (unsigned rcount : section_counts) {

        //  For each resource record in the current section....
        for (unsigned rr_ix = 0; rr_ix < rcount; ++rr_ix) {

            //  Parse the name field.
            std::string name;
            int label_length;
            while ((label_length = *ptr++)) {
                if (ptr >= end)
                    return Disposition::TRUNCATED;
                //  Deal with label compression.
                if (label_length & 0xc0) {
                    if (ptr >= end)
                        return Disposition::TRUNCATED;
                    if ((label_length & 0xc0) != 0xc0)
                        return Disposition::DNS_ERROR;
                    int pointer_offset = ((label_length & 0x3f) << 8) + *ptr++;
                    const unsigned char* ptr2 = payload + pointer_offset;
                    if (ptr2 >= end)
                        return Disposition::TRUNCATED;
                    int label_length;
                    while ((label_length = *ptr2++)) {
                        //  Max label length is 63 octets.
                        if ((label_length & 0xc0) == 0xc0) {
                            int pointer_offset = ((label_length & 0x3f) << 8) + *ptr2++;
                            ptr2 = payload + pointer_offset;
                            continue;
                        }
                        if (label_length >= 0xc0)
                            return Disposition::DNS_ERROR;
                        if (ptr2 >= end)
                            return Disposition::TRUNCATED;
                        if (name.size())
                            name.push_back('.');
                        name.append(ptr2, ptr2+label_length);
                        ptr2 += label_length;
                        if (ptr2 >= end)
                            return Disposition::TRUNCATED;
                    }
                    break;
                }
                else {
                    if (name.size())
                        name.push_back('.');
                    name.append(ptr, ptr+label_length);
                    ptr += label_length;
                    if (ptr >= end)
                        return Disposition::TRUNCATED;
                }
            }

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

            if (rclass == 1 && type == 1) {
                if (rdlength != 4) // A record should have a 4-octet IP address.
                    return Disposition::DNS_ERROR;
                IPV4Address ipaddr;
                std::copy_n(rdata_start, 4, ipaddr.begin());
                snoop.get_model().note_name(ipaddr, name, Model::NameType::DNS);
            }
        }
    }

    return Disposition::DNS;
}
