#pragma once


#include <ostream>
#include <time.h>

#include "util.hpp"
#include "Disposition.hpp"
#include "Model.hpp"
#include "UDPSession.hpp"


class Snoop
{
public:
    struct Stats {
        long observed = 0;
        long dispositions[int(Disposition::_MAX)] = { 0 };
    };

    Snoop();
    void parse_ethernet(const timeval& ts, const unsigned char* frame, unsigned frame_length);
    const Stats& get_stats() const { return stats; }
    Model& get_model() { return model; }

private:
    Stats stats;
    Model model;

    Disposition _parse_ethernet(const unsigned char* frame, unsigned frame_length);
    Disposition parse_arp(const unsigned char* frame, unsigned frame_length);
    Disposition parse_ipv4(const MacAddress& eth_src_addr,
                           const MacAddress& eth_dst_addr,
                           const unsigned char* packet,
                           unsigned packet_length);
    Disposition parse_udp(const IPV4Address& src,
                          const IPV4Address& dst,
                          const unsigned char* packet,
                          unsigned packet_length);

    std::map<IPV4UDPKey, IPV4UDPSession> ipv4_udp_sessions;
};


std::ostream& operator<<(std::ostream&, const Snoop::Stats&);
