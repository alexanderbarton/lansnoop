#include "Snoop.hpp"

#include <algorithm>
#include <array>
#include <iomanip>

#include <net/ethernet.h>
#include <net/if_arp.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
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
        Disposition disp = _parse_ethernet(frame, frame_length);
        this->stats.dispositions[int(disp)]++;
    }
}


Disposition Snoop::_parse_ethernet(const unsigned char* frame, unsigned frame_length)
{
    if (frame_length < sizeof(struct ether_header))
        return Disposition::TRUNCATED;

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

    MacAddress source, destination;
    std::copy_n(header->ether_shost, 6, source.begin());
    std::copy_n(header->ether_dhost, 6, destination.begin());
    this->model.note_l2_packet_traffic(source, destination);

    const unsigned char* payload = frame + sizeof(ether_header);
    unsigned payload_length = frame_length - sizeof(ether_header);

    switch (ntohs(header->ether_type)) {
        case 0x0800: // IPv4
            return parse_ipv4(source, destination, payload, payload_length);
        case 0x0806: // ARP
            return parse_arp(payload, payload_length);
        default:
            return Disposition::ETHERTYPE_BAD;
    }

    return Disposition::DISINTEREST;
}


Disposition Snoop::parse_arp(const unsigned char* frame, unsigned frame_length)
{
    const struct arphdr* header = reinterpret_cast<const struct arphdr*>(frame);
    if (frame_length < sizeof(*header))
        return Disposition::TRUNCATED;

    if (ntohs(header->ar_hrd) != ARPHRD_ETHER)
        return Disposition::ARP_DISINTEREST; // We don't expect to see anything other than Ethernet.
    if (ntohs(header->ar_pro) != 0x0800)
        return Disposition::ARP_DISINTEREST; // We don't expect to see anything other than IPv4.
    if (header->ar_hln != 6)
        return Disposition::ARP_ERROR; // Expect 6-byte MAC addresses.
    if (header->ar_pln != 4)
        return Disposition::ARP_ERROR; // Protocol address length should be 4 for IPv4.

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
            return Disposition::ARP_DISINTEREST; // Not implemented.

        default:
            return Disposition::ARP_ERROR; // Bad ARP opcode?
    }

    struct args_st {
        unsigned char sender_mac[6];
        unsigned char sender_ip_address[4];
        unsigned char target_mac[6];
        unsigned char target_ip_address[4];
    };
    const struct args_st* args = reinterpret_cast<const args_st*>(frame+sizeof(*header));
    if (sizeof(*header) + sizeof(*args) > frame_length)
        return Disposition::TRUNCATED;

    MacAddress mac;
    IPV4Address ipaddr;
    std::copy(args->sender_mac, args->sender_mac+6, mac.begin());
    std::copy(args->sender_ip_address, args->sender_ip_address+4, ipaddr.begin());
    this->model.note_arp(mac, ipaddr);

    std::copy(args->target_mac, args->target_mac+6, mac.begin());
    std::copy(args->target_ip_address, args->target_ip_address+4, ipaddr.begin());
    this->model.note_arp(mac, ipaddr);

    return Disposition::ARP;
}


Disposition Snoop::parse_ipv4(
        const MacAddress& eth_src_addr,
        const MacAddress& eth_dst_addr,
        const unsigned char* packet,
        unsigned packet_length)
{
    if (packet_length < sizeof(struct ip))
        return Disposition::TRUNCATED;

    const struct ip* header = reinterpret_cast<const struct ip*>(packet);

    uint16_t frag_off = ntohs(header->ip_off) & IP_OFFMASK;
    unsigned more_fragments = (ntohs(header->ip_off) & IP_MF) == IP_MF;
    if (frag_off || more_fragments)
        return Disposition::IPv4_FRAGMENT;
    //  TODO: handle fragments

    if (4 != header->ip_v)
        return Disposition::IPv4_BAD;

    //  total_length is IP header + IP payload.  It may be less than
    //  this frame's length due to padding.  It may be more than
    //  this frame's length due to truncation.
    //
    uint16_t total_length = ntohs(header->ip_len);
    uint16_t adjusted_length = packet_length;
    if (total_length < adjusted_length)
        adjusted_length = total_length;
    else if (total_length > adjusted_length)
        return Disposition::TRUNCATED;

    IPV4Address ip_src_addr;
    const unsigned char* s_addr = reinterpret_cast<const unsigned char*>(&header->ip_src.s_addr);
    std::copy(s_addr, s_addr+4, ip_src_addr.begin());
    this->model.note_ip_through_interface(ip_src_addr, eth_src_addr);

    IPV4Address ip_dst_addr;
    s_addr = reinterpret_cast<const unsigned char*>(&header->ip_dst.s_addr);
    std::copy(s_addr, s_addr+4, ip_dst_addr.begin());
    this->model.note_ip_through_interface(ip_dst_addr, eth_dst_addr);

    unsigned int header_length = 4 * header->ip_hl;
    switch (header->ip_p) {
#if 0
        case IPPROTO_TCP:
            parse_tcp(packet + header_length, adjusted_length - header_length);
            break;
#endif

        case IPPROTO_UDP:
            return parse_udp(ip_src_addr, ip_dst_addr, packet + header_length, adjusted_length - header_length);
            break;

        default:
            return Disposition::IPv4_PROTOCOL;
            break;
    }
}


Disposition Snoop::parse_udp(
        const IPV4Address& src_ip,
        const IPV4Address& dst_ip,
        const unsigned char* packet,
        unsigned length)
{
    if (length < sizeof(struct udphdr))
        return Disposition::TRUNCATED;

    const struct udphdr* header = reinterpret_cast<const struct udphdr*>(packet);
    IPV4SockAddress src_sa { src_ip, ntohs(header->uh_sport) };
    IPV4SockAddress dst_sa { dst_ip, ntohs(header->uh_dport) };

    IPV4UDPKey key(src_sa, dst_sa);
    auto it = this->ipv4_udp_sessions.find(key);
    if (it == this->ipv4_udp_sessions.end()) {
        auto [new_it, _] = this->ipv4_udp_sessions.emplace(key, key);
        it = new_it;
    }
    int dir = src_sa == key.a;
    return it->second.put(*this, dir, packet+sizeof(struct udphdr), length - sizeof(struct udphdr));
}


//  TODO: Implement IPv6's Neighbor Discovery and Inverse Neighbor Discovery protocols.


std::ostream& operator<<(std::ostream& o, const Snoop::Stats& stats)
{
    o << "Stats:\n";
    o << "    " << std::setw(9) << stats.observed << " packets observed\n";
    o << "    " << "         " << " packet dispositions\n";
    for (int i=0; i<int(Disposition::_MAX); ++i) {
        o << "       " << std::setw(9) << stats.dispositions[i] << " " << Disposition(i) << "\n";
    }
    return o;
}
