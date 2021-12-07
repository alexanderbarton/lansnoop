#pragma once


#include <ostream>
#include <time.h>

#include "Model.hpp"


class Snoop
{
public:
    enum class DISPOSITION {
        TRUNCATED,
        PROCESSED,
        _MAX,
    };

    struct Stats {
        long observed = 0;
        long dispositions[int(DISPOSITION::_MAX)] = { 0 };
    };

    Snoop();
    void parse_ethernet(const timeval& ts, const unsigned char* frame, unsigned frame_length);
    const Stats& get_stats() const { return stats; }
    Model& get_model() { return model; }

private:
    Stats stats;
    Model model;

    DISPOSITION _parse_ethernet(const unsigned char* frame, unsigned frame_length);
};


std::ostream& operator<<(std::ostream&, const Snoop::Stats&);
std::ostream& operator<<(std::ostream&, Snoop::DISPOSITION);
