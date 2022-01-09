#include <fstream>
#include <stdexcept>
#include <array>
#include <algorithm>

#include <sys/types.h>
#include <regex.h>
#include <arpa/inet.h>
#include <ctype.h>

#include "IPV4PrefixTable.hpp"

#include "/home/abarton/debug.hpp"


void IPV4PrefixTable::load(const std::string& path, bool verbose)
{
#if 0
    //  Regex to match lines like "1.0.0.0/24      13335".
    regex_t regex;
    int ret = regcomp(&regex, "^\\([0-9.]\\+\\)/\\([0-9]\\+\\)	\\([0-9]\\+\\)$", 0);
    if (ret) {
        show(ret);
        throw std::runtime_error("regex compilation failed");
    }
#endif

    std::ifstream in(path);
    if (!in)
        throw std::invalid_argument("unable to open prefix file");

    int count = 0;
    char line[1024];
    std::array<regmatch_t, 4> matches;
    while (in.getline(line, sizeof(line), '\n')) {
        ++count;
#if 0
        //  This is the "right" way to do this...
        int ret = regexec(&regex, line, matches.size(), matches.data(), 0);
        if (ret)
            throw std::invalid_argument("parse error");
#else
        //  ...but this is 10x faster.  Hmm.
        int ix = 0;
        matches[1].rm_so = ix;
        while (line[ix] && (isdigit(line[ix]) || line[ix] == '.'))
            ++ix;
        if (!line[ix])
            throw std::invalid_argument("parse error");
        matches[1].rm_eo = ix;
        if (line[ix++] != '/')
            throw std::invalid_argument("parse error");

        matches[2].rm_so = ix;
        while (line[ix] && isdigit(line[ix]))
            ++ix;
        if (!line[ix])
            throw std::invalid_argument("parse error");
        matches[2].rm_eo = ix;

        if (line[ix++] != '\t')
            throw std::invalid_argument("parse error");
        matches[3].rm_so = ix;
        while (line[ix] && isdigit(line[ix]))
            ++ix;
        matches[3].rm_eo = ix;
#endif

        std::string address_s(line + matches[1].rm_so, line + matches[1].rm_eo);
        in_addr address_st;
        if (!inet_pton(AF_INET, address_s.c_str(), &address_st))
            throw std::invalid_argument("failed parsing IP address");
        uint32_t address = ntohl(address_st.s_addr);

        int shift = std::stoi(std::string(line + matches[2].rm_so, line + matches[2].rm_eo));
        if (shift > 32)
            throw std::invalid_argument("parse error");
        uint32_t netmask = 0xffffffffUL << (32-shift);
        if ((address & netmask) != address)
            throw std::invalid_argument("parse error");

        uint32_t asn = std::stol(std::string(line + matches[3].rm_so, line + matches[3].rm_eo));
        if (asn == 0U || asn == 65535U) // Reserved ASNs.
            throw std::invalid_argument("parse error");

        this->prefixes.push_back(Prefix { address, netmask, asn });
    }

    std::sort(this->prefixes.begin(), this->prefixes.end());

#if 0
    regfree(&regex);
#endif
    if (verbose)
        std::cerr << count << " IPv4 prefixes loaded from " << path << std::endl;

#if TEST
    for (uint32_t ip : { 0x00000001U, 0x000000ffU, 0x01000000U, 0x01000001U, 0x010000ffU, 0x010003ffU, 0x01010600U,
                         0xdffff000U, 0xdffff100U, 0xdffffe00U, 0xffffffffU }) {
        showx(ip);
        const Prefix* prefix = look_up(ip);
        if (prefix) {
            showx(prefix->address);
            showx(prefix->netmask);
            show(prefix->asn);
        }
    }
#endif
}


const IPV4PrefixTable::Prefix* IPV4PrefixTable::look_up(uint32_t address) const
{
    //  The prefix file occasionally contains overlapping entries like:
    //      223.255.240.0/22        55649
    //      223.255.240.0/24        55649
    //      223.255.241.0/24        55649
    //  This code may return the first or third prefix above as a match for 223.255.241.
    //  TODO: Deal with this somehow.

    int l = 0; //  The start of our current search range.
    int r = this->prefixes.size(); // One past the end of our current search range.
    while (l < r) {
        int m = (l + r) / 2;
        const Prefix& prefix = this->prefixes[m];
        if ((prefix.netmask & address) == prefix.address)
            return &this->prefixes[m];
        else if (address > prefix.address)
            l = m+1;
        else
            r = m;
    }
    return nullptr;
}


const IPV4PrefixTable::Prefix* IPV4PrefixTable::look_up(const IPV4Address& address) const
{
    uint32_t a = 0;
    for (int i=0; i<4; ++i)
        a = (a << 8) | address[i];
    return look_up(a);
}
