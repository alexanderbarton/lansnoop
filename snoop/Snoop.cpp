#include "Snoop.hpp"

#include <algorithm>
#include <array>
#include <iomanip>

#include <net/ethernet.h>


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

    return DISPOSITION::PROCESSED;
}


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
        case Snoop::DISPOSITION::TRUNCATED: return o << "TRUNCATED";
        case Snoop::DISPOSITION::PROCESSED: return o << "PROCESSED";
        case Snoop::DISPOSITION::_MAX: return o << "_MAX";
        default: return o << "(invalid)";
    };
}
