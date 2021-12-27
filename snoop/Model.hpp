#pragma once

#include <map>
#include <set>
#include <array>
#include <ostream>


class Model {
public:
    typedef std::array<unsigned char, 6> MacAddress;

    struct Network {
        long id;
        std::set<long> interfaces;
    };

    struct Interface {
        long id;
        std::array<unsigned char, 6> address;  // MAC address
        long network_id; //  All interfaces belong to exactly one network.
        std::string maker;
        long packet_count = 0; // Number of Ethernet frames addresses to or from this interface.
    };

    void note_time(long t);
    void note_packet_count(long c) { this->packet_count = c; }

    //  Note one Ethernet packet traversing between two interfaces.
    void note_l2_packet_traffic(const MacAddress& source_address,
                                const MacAddress& destination_address);

    //  Generate a topology report.
    void report(std::ostream&) const;

    //  Load the OUI table from a CSV file.
    //  Obtain from http://standards-oui.ieee.org/oui/oui.csv
    void load_oui(const std::string& path);

private:

    long now = 0; //  Nanoseconds since the epoch.
    long packet_count = 0;

    //  Unique ID generator.
    long next_id = 0;

    //  Maps network ID's to network instances.
    std::map<long, Network> networks;

    //  Maps an interface's MAC address to its Interface instance.
    std::map<MacAddress, Interface> interfaces_by_address;

    //  Set of interface ID's that have had packet traffic since the last traffic update.
    std::set<MacAddress> recent_interface_traffic;
    long last_traffic_update = 0;

    //  Maps an interface's ID to its address.
    std::map<long, MacAddress> interfaces_by_id;

    //  Map OUIs to organization names.
    std::map<int, std::string> ouis;


    long new_network();

    std::map<MacAddress, Interface>::iterator new_interface(const MacAddress& address, long network_id);

    //  Merge two networks.
    void merge_networks(long a_id, long b_id);

    void emit(const Network& network, bool fini = false);
    void emit(const Interface& interface, bool fini = false);
    void emit_interface_traffic_update();
};


std::ostream& operator<<(std::ostream&, Model::MacAddress);
