#pragma once

#include <map>
#include <set>
#include <vector>
#include <array>
#include <ostream>


class Model {
public:
    class MacAddress : public std::array<unsigned char, 6> {};  // Network byte order.
    class IPV4Address : public std::array<unsigned char, 4> {};  // Network byte order.

    struct Network {
        long id;
        std::set<long> interfaces;
    };

    struct Interface {
        long id;
        MacAddress address;  // MAC address
        long network_id; //  All interfaces belong to exactly one network.
        std::string maker;
        long packet_count = 0; // Number of Ethernet frames addresses to or from this interface.
    };

    struct IPAddressInfo {
        long id;
        IPV4Address address;
        long interface_id;  //  Iff not 0, this IP address is attached to this interface.
    };

    void note_time(long t);
    void note_packet_count(long c) { this->packet_count = c; }

    //  Note one Ethernet packet traversing between two interfaces.
    void note_l2_packet_traffic(const MacAddress& source_address,
                                const MacAddress& destination_address);

    void note_arp(const MacAddress& mac_address, const IPV4Address& ip_address);

    //  Generate a topology report.
    void report(std::ostream&) const;

    //  Load the OUI table from a CSV file.
    //  Obtain from http://standards-oui.ieee.org/oui/oui.csv
    void load_oui(const std::string& path);

private:

    long now = 0; //  Nanoseconds since the epoch.
    long packet_count = 0;

    //  Unique ID generator.
    //  First ID is 1 because, in some cases, 0 means "none".
    long next_id = 1;

    //  Maps network ID's to network instances.
    std::map<long, Network> networks;

    //  Maps an interface's MAC address to its Interface instance.
    std::map<MacAddress, Interface> interfaces_by_address;

    //  Set of interface ID's that have had packet traffic since the last traffic update.
    std::set<MacAddress> recent_interface_traffic;
    long last_traffic_update = 0;

    //  Maps an interface's ID to its address.
    std::map<long, MacAddress> interfaces_by_id;

    //  Maps IP addresses to an IPAddressInfos.
    std::map<IPV4Address, IPAddressInfo> ip_addresses;

    //  Map OUIs to organization names.
    std::map<int, std::string> ouis;


    long new_network();

    std::map<MacAddress, Interface>::iterator new_interface(const MacAddress& address, long network_id);
    IPAddressInfo& new_ip_address(const IPV4Address& address, long interface_id);

    //  Merge two networks.
    void merge_networks(long a_id, long b_id);

    void emit(const Network&, bool fini = false);
    void emit(const Interface&, bool fini = false);
    void emit(const IPAddressInfo&, bool fini = false);
    void emit_interface_traffic_update();
};


std::ostream& operator<<(std::ostream&, const Model::MacAddress&);
std::ostream& operator<<(std::ostream&, const Model::IPV4Address&);
