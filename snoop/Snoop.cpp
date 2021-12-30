#include "Snoop.hpp"

#include <algorithm>
#include <array>
#include <iomanip>

#include <net/ethernet.h>
#include <net/if_arp.h>
#include <arpa/inet.h>

#include "/home/abarton/debug.hpp"


Snoop::Snoop()
{
}


void Snoop::parse_ethernet(const timeval& ts, const unsigned char* frame, unsigned frame_length)
{
    this->model.note_time(ts.tv_sec * 1e9 + ts.tv_usec * 1e3);
    if (frame) {
        this->stats.observed++;
        this->model.note_packet_count(this->stats.observed);
        DISPOSITION disp = _parse_ethernet(frame, frame_length);
        this->stats.dispositions[int(disp)]++;
    }
}


Snoop::DISPOSITION Snoop::_parse_ethernet(const unsigned char* frame, unsigned frame_length)
{
    if (frame_length < sizeof(struct ether_header))
        return DISPOSITION::TRUNCATED;

    const struct ether_header* header = reinterpret_cast<const struct ether_header*>(frame);

    static_assert(sizeof(header->ether_shost) <= sizeof(long), "48-bit MAC address should fit into a 64-bit long");
#if 0
    long source { 0 };
    for (const auto o : header->ether_shost)
        source = (source << 8) | o;
    long destination { 0 };
    for (const auto o : header->ether_dhost)
        destination = (destination << 8) | o;
#endif

    Model::MacAddress source, destination;
    std::copy_n(header->ether_shost, 6, source.begin());
    std::copy_n(header->ether_dhost, 6, destination.begin());
    this->model.note_l2_packet_traffic(source, destination);

    const unsigned char* payload = frame + sizeof(ether_header);
    unsigned payload_length = frame_length - sizeof(ether_header);

    switch (ntohs(header->ether_type)) {
        case 0x0806: // ARP
            return parse_arp(payload, payload_length);
    }

    return DISPOSITION::DISINTEREST;
}


Snoop::DISPOSITION Snoop::parse_arp(const unsigned char* frame, unsigned frame_length)
{
    const struct arphdr* header = reinterpret_cast<const struct arphdr*>(frame);
    if (frame_length < sizeof(*header))
        return DISPOSITION::TRUNCATED;

    if (ntohs(header->ar_hrd) != ARPHRD_ETHER)
        return DISPOSITION::ARP_DISINTEREST; // We don't expect to see anything other than Ethernet.
    if (ntohs(header->ar_pro) != 0x0800)
        return DISPOSITION::ARP_DISINTEREST; // We don't expect to see anything other than IPv4.
    if (header->ar_hln != 6)
        return DISPOSITION::ARP_ERROR; // Expect 6-byte MAC addresses.
    if (header->ar_pln != 4)
        return DISPOSITION::ARP_ERROR; // Protocol address length should be 4 for IPv4.

    switch (ntohs(header->ar_op)) {

        case ARPOP_REPLY:
            //  Handle below.
            break;

        case ARPOP_REQUEST:
        case ARPOP_RREQUEST:
        case ARPOP_RREPLY:
        case ARPOP_InREQUEST:
        case ARPOP_InREPLY:
        case ARPOP_NAK:
            return DISPOSITION::ARP_DISINTEREST; // Not implemented.

        default:
            return DISPOSITION::ARP_ERROR; // Bad ARP opcode?
    }

    struct args_st {
        unsigned char sender_mac[6];
        unsigned char sender_ip_address[4];
        unsigned char target_mac[6];
        unsigned char target_ip_address[4];
    };
    const struct args_st* args = reinterpret_cast<const args_st*>(frame+sizeof(*header));
    if (sizeof(*header) + sizeof(*args) > frame_length)
        return DISPOSITION::TRUNCATED;

    Model::MacAddress mac;
    Model::IPV4Address ipaddr;
    std::copy(args->sender_mac, args->sender_mac+6, mac.begin());
    std::copy(args->sender_ip_address, args->sender_ip_address+4, ipaddr.begin());
    this->model.note_arp(mac, ipaddr);

    std::copy(args->target_mac, args->target_mac+6, mac.begin());
    std::copy(args->target_ip_address, args->target_ip_address+4, ipaddr.begin());
    this->model.note_arp(mac, ipaddr);

    return DISPOSITION::ARP; // Not implemented.
}


//  TODO: Implement IPv6's Neighbor Discovery and Inverse Neighbor Discovery protocols.


std::ostream& operator<<(std::ostream& o, const Snoop::Stats& stats)
{
    o << "Stats:\n";
    o << "    " << std::setw(9) << stats.observed << " packets observed\n";
    o << "    " << "         " << " packet dispositions\n";
    for (int i=0; i<int(Snoop::DISPOSITION::_MAX); ++i) {
        o << "       " << std::setw(9) << stats.dispositions[i] << " " << Snoop::DISPOSITION(i) << "\n";
    }
    return o;
}


std::ostream& operator<<(std::ostream& o, Snoop::DISPOSITION d)
{
    switch (d) {
        case Snoop::DISPOSITION::TRUNCATED:       return o << "TRUNCATED";
        case Snoop::DISPOSITION::PROCESSED:       return o << "PROCESSED";
        case Snoop::DISPOSITION::ERROR:           return o << "ERROR";
        case Snoop::DISPOSITION::DISINTEREST:     return o << "DISINTEREST";
        case Snoop::DISPOSITION::ARP:             return o << "ARP";
        case Snoop::DISPOSITION::ARP_DISINTEREST: return o << "ARP_DISINTEREST";
        case Snoop::DISPOSITION::ARP_ERROR:       return o << "ARP_ERROR";
        case Snoop::DISPOSITION::_MAX:            return o << "_MAX";
        default: return o << "(invalid)";
    };
}
