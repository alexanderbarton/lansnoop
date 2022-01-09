#include <ostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include "Model.hpp"
#include "event.pb.h"
#include "EventSerialization.hpp"

#include "/home/abarton/debug.hpp"


void Model::note_time(long t)
{
    this->now = t;
    const long millisecond = 1000000L;
    if (this->now >= this->last_traffic_update + 10*millisecond) {
        if (this->recent_interface_traffic.size()) {
            emit_traffic_update();
            this->recent_interface_traffic.clear();
            this->recent_cloud_traffic.clear();
            this->recent_ipaddress_traffic.clear();
        }
        this->last_traffic_update = this->now + 10*millisecond;
    }
}


void Model::note_l2_packet_traffic(const MacAddress& source_address,
                                   const MacAddress& destination_address)
{
    auto source_i = this->interfaces_by_address.find(source_address);
    auto destination_i = this->interfaces_by_address.find(destination_address);
    bool is_multicast = destination_address[0] & 0x01;

    //
    //  Check for topology updates.
    //
    if (is_multicast) {
        //  In a multicast broadcast, only the source interface is "real".
        //
        if (source_i == this->interfaces_by_address.end()) {
#if 0
            long network_id = this->new_network();
#else
            //  Assume all interfaces are on the same one network.
            long network_id;
            if (this->networks.size())
                network_id = this->networks.begin()->first;
            else
                network_id = this->new_network();
#endif
            source_i = this->new_interface(source_address, network_id);
        }
    }
    else {
        //  Both interfaces are known to us.
        //
        if (source_i != this->interfaces_by_address.end() && destination_i != this->interfaces_by_address.end()) {
            if (source_i->second.network_id != destination_i->second.network_id)
                this->merge_networks(source_i->second.network_id, destination_i->second.network_id);
        }
        //  Both interfaces are new to us.
        else if (source_i == this->interfaces_by_address.end() && destination_i == this->interfaces_by_address.end()) {
#if 0
            long network_id = this->new_network();
#else
            //  Assume all interfaces are on the same one network.
            long network_id;
            if (this->networks.size())
                network_id = this->networks.begin()->first;
            else
                network_id = this->new_network();
#endif
            source_i = this->new_interface(source_address, network_id);
            destination_i = this->new_interface(destination_address, network_id);
        }
        //  Only the source interface is new.
        else if (source_i == this->interfaces_by_address.end()) {
            source_i = this->new_interface(source_address, destination_i->second.network_id);
        }
        //  Only the destination interface is new.
        else {
            destination_i = this->new_interface(destination_address, source_i->second.network_id);
        }
    }

    //  Update packet counters.
    //
    if (source_i != this->interfaces_by_address.end()) {
        this->recent_interface_traffic.insert(source_i->second.address);
        source_i->second.packet_count++;
    }
    if (destination_i != this->interfaces_by_address.end()) {
        this->recent_interface_traffic.insert(destination_i->second.address);
        destination_i->second.packet_count++;
    }
}


void Model::note_ip_through_interface(const IPV4Address& ip, const MacAddress& mac)
{
    if (mac[0] & 0x01)
        return;  // Multicast address.

    IPAddressInfo* ipaddressinfo;
    auto ip_it = ip_addresses.find(ip);
    if (ip_it != this->ip_addresses.end()) {
        //  We already know about this IP address.
        //  TODO: check that this IP address is still in the right place?
        ipaddressinfo = &ip_it->second;
    }
    else {
        //  Create a new IPAddr instance and assign it to the interface's attached Cloud.
        auto interface_it = this->interfaces_by_address.find(mac);
        if (interface_it == this->interfaces_by_address.end())
            throw std::invalid_argument("note_ip_through_interface(): mac address not found");
        const Interface& interface = interface_it->second;

        if (!this->cloud_ids_by_interface_addresses.count(mac))
            this->new_cloud(interface);
        long cloud_id = this->cloud_ids_by_interface_addresses.at(mac);
        Cloud& cloud = this->clouds.at(cloud_id);

        ipaddressinfo = &this->new_ip_address(ip, cloud);
    }

    //  Increment packet counts.
    //
    ++ipaddressinfo->packet_count;
    this->recent_ipaddress_traffic.insert(ipaddressinfo->address);

    long cloud_id = ipaddressinfo->cloud_id;
    while (cloud_id) {
        Cloud& cloud = this->clouds.at(cloud_id);
        ++cloud.packet_count;
        this->recent_cloud_traffic.insert(cloud.id);
        cloud_id = cloud.cloud_id;
    }
}


//  Assign an IP address to an interface.
//
void Model::note_arp(const MacAddress& mac_address, const IPV4Address& ip_address)
{
    auto interface_i = this->interfaces_by_address.find(mac_address);
    if (interface_i == this->interfaces_by_address.end())
        return;  // We *should* find this.
    const Interface& interface = interface_i->second;

    auto ip_address_i = this->ip_addresses.find(ip_address);
    if (ip_address_i == this->ip_addresses.end()) {
        //  Create a new IPAddressInfo instance.
        this->new_ip_address(ip_address, interface.id);
    }
    else {
        //  Update an existing IPAdressInfo to assign it to a (new) interface.
        ip_address_i->second.interface_id = interface.id;
        ip_address_i->second.cloud_id = 0;
        emit(ip_address_i->second);
    }
}


void Model::note_name(const IPV4Address& address, const std::string& name, NameType type)
{
    this->ipv4_address_names[address].insert(NameEntry { name, type });

    auto it = this->ip_addresses.find(address);
    if (it != this->ip_addresses.end()) {
        IPAddressInfo& addrinfo = it->second;
        if (addrinfo.ns_name != name) {
            addrinfo.ns_name = name;
            emit(addrinfo);
        }
    }
}


void Model::report(std::ostream& o) const
{
    for (const auto& entry : this->networks) {
        const Network& network = entry.second;
        o << "Network " << network.id << "\n";
        for (long interface_id : network.interfaces) {
            const MacAddress& address = this->interfaces_by_id.find(interface_id)->second;
            const Interface& interface = this->interfaces_by_address.find(address)->second;
            o << "    Interface " << interface.id << "\n";
            o << "        address:    " << interface.address << "\n";
            o << "        network ID: " << interface.network_id << "\n";
            o << "        maker:      " << interface.maker << "\n";
        }
    }
    for (const auto& entry : this->ip_addresses) {
        const IPAddressInfo& ip_address = entry.second;
        o << "IPAddressInfo " << ip_address.id << "\n";
        o << "    address:      " << ip_address.address << "\n";
        if (ip_address.interface_id)
            o << "    interface_id: " << ip_address.interface_id << "\n";
        else
            o << "    interface_id: " << "(none)" << "\n";
        if (ip_address.cloud_id)
            o << "    cloud_id:     " << ip_address.cloud_id << "\n";
        else
            o << "    cloud_id:     " << "(none)" << "\n";
        o << "    ns_name:      " << ip_address.ns_name << "\n";
        if (ip_address.asn)
            o << "    asn:          " << ip_address.asn << "\n";
        else
            o << "    asn:          " << "\n";
        o << "    as_name:      " << ip_address.as_name << "\n";
    }
    for (const auto& [id, cloud] : this->clouds) {
        o << "Cloud: " << id << "\n";
        o << "    description:  " << cloud.description << "\n";
        o << "    interface_id: " << cloud.interface_id << "\n";
        o << "    cloud_id:     " << cloud.cloud_id << "\n";
        o << "    children:    ";
        for (long child_id : cloud.child_cloud_ids)
            o << " " << child_id;
        o << "\n";

    }
    o << "\nName Service Name Table\n";
    for (const auto& [address, nameset] : this->ipv4_address_names) {
        o << "    " << address << ":";
        for (const NameEntry& ne : nameset)
            o << " " << ne.name;
        o << "\n";
    }
}


void Model::load_oui(const std::string& path, bool verbose)
{
    //  This seems to be an ISO-4180 formatted file.

    std::ifstream in(path);
    if (!in.good())
        throw std::invalid_argument("unable to open OUI CSV file");

    int count = 0;
    char line[1024];
    while (in.getline(line, sizeof(line), '\n')) {
        if (!count++)
            continue; // Skip column header line.
        char* ptr = line;

        //  Discard first column.
        while (*ptr && *ptr != ',')
            ptr++;
        if (*ptr != ',')
            throw std::invalid_argument("OUI parse error, missing first comma");
        ptr++;

        //  Second column should contain a 6-digit hex OUI value.
        int oui = 0;
        for (int i=0; i<6; ++i) {
            int digit = tolower(*ptr++);
            if (digit >= 'a')
                digit = digit - 'a' + 10;
            else
                digit -= '0';
            oui = (oui << 4) + digit;
        }
        if (*ptr++ != ',')
            throw std::invalid_argument("OUI parse error, expected second comma");

        //  Third column is organization name.
        bool quoted = (*ptr == '"');
        if (quoted)
            ++ptr;
        char* name_start = ptr;
        if (quoted) {
            //  A "" inside the field is a single quote, and not a field delimiter.
            while (*ptr) {
                if (ptr[0] == '"' && ptr[1] == '"')
                    ptr += 2;
                else if (*ptr == '"')
                    break;
                else
                    ++ptr;
            }
            if (*ptr != '"')
                throw std::invalid_argument("OUI parse error, third column, unmatched quote");
        }
        else {
            while (*ptr && *ptr != ',')
                ++ptr;
            if (*ptr != ',')
                throw std::invalid_argument("OUI parse error, expected third comma");
        }

        std::string org_name(name_start, ptr);

        //  Discard the remaining columns.

        this->ouis[oui] = org_name;
    }
    if (verbose)
        std::cerr << count << " OUIs loaded from " << path << std::endl;
}


void Model::load_prefixes(const std::string& path, bool verbose)
{
    this->prefixes.load(path, verbose);
}


void Model::load_asns(const std::string& path, bool verbose)
{
    std::ifstream in(path);
    if (!in.good())
        throw std::invalid_argument("unable to open ASN file");

    int count = 0;
    char line[1024];
    while (in.getline(line, sizeof(line), '\n')) {
        ++count;
        char* ptr = line;

        //  Read past leading whitespace.
        while (*ptr == ' ')
            ++ptr;

        uint32_t asn = 0;
        while (*ptr && isdigit(*ptr))
            asn = asn * 10 + *ptr++ - '0';

        //  Whitespace.
        while (*ptr == ' ')
            ++ptr;

        this->asns[asn] = ptr;
    }
    if (verbose)
        std::cerr << count << " ASNs loaded from " << path << std::endl;
}


//  Create a new network.
long Model::new_network()
{
    long id = this->next_id++;
    Network& network = this->networks[id];  //  Implicit insert.
    network.id = id;
    emit(network);
    return network.id;
}


//  Create a new interface.
//  Assign it to the provided network.
std::map<MacAddress, Model::Interface>::iterator Model::new_interface(const MacAddress& address, long network_id)
{
    int oui = (int(address[0]) << 16) | (int(address[1]) << 8) | int(address[2]);
    // show(this->ouis.size());
    // showx(oui);
    auto ouis_i = this->ouis.find(oui);

    Interface interface;
    interface.address = address;
    interface.id = this->next_id++;
    interface.network_id = network_id;
    if (ouis_i != this->ouis.end())
        interface.maker = ouis_i->second;
    this->interfaces_by_address[address] = interface;
    this->interfaces_by_id[interface.id] = address;
    this->networks[network_id].interfaces.insert(interface.id);

    emit(interface);

    return this->interfaces_by_address.find(interface.address);
}


Model::IPAddressInfo& Model::new_ip_address(const IPV4Address& address, long interface_id)
{
    if (this->ip_addresses.count(address))
        throw std::invalid_argument("new_ip_address(): address already exists");
    IPAddressInfo& ip_address_info = this->ip_addresses[address];;
    ip_address_info.id = this->next_id++;
    ip_address_info.address = address;
    ip_address_info.interface_id = interface_id;  // Initially not assigned to any interface.

    const IPV4PrefixTable::Prefix* prefix = this->prefixes.look_up(address);
    if (prefix) {
        ip_address_info.asn = prefix->asn;
        auto it = this->asns.find(ip_address_info.asn);
        if (it != this->asns.end())
            ip_address_info.as_name = it->second;
    }

    emit(ip_address_info);

    return ip_address_info;
}


Model::IPAddressInfo& Model::new_ip_address(const IPV4Address& address, Cloud& cloud)
{
    if (this->ip_addresses.count(address))
        throw std::invalid_argument("new_ip_address(): address already exists");
    IPAddressInfo& ip_address_info = this->ip_addresses[address];;
    ip_address_info.id = this->next_id++;
    ip_address_info.address = address;
    ip_address_info.interface_id = 0;
    ip_address_info.cloud_id = cloud.id;
    
    //  Do we have a name server name for this IP address?
    auto ns_it = this->ipv4_address_names.find(address);
    if (ns_it != this->ipv4_address_names.end())
        ip_address_info.ns_name = ns_it->second.begin()->name;

    //  Do we have an ASN for this IP address?
    const IPV4PrefixTable::Prefix* prefix = this->prefixes.look_up(address);
    if (prefix) {
        ip_address_info.asn = prefix->asn;
        auto it = this->asns.find(ip_address_info.asn);
        if (it != this->asns.end())
            ip_address_info.as_name = it->second;
    }

    //  If this address has a known ASN owner, attach this address
    //  to a subcloud named after the ASN.
    //
    if (ip_address_info.as_name.size()) {
        //  Find an existing subcloud.
        for (long child_id : cloud.child_cloud_ids) {
            Cloud& child_cloud = this->clouds.at(child_id);
            if (child_cloud.description == ip_address_info.as_name) {
                ip_address_info.cloud_id = child_cloud.id;
                break;
            }
        }
        //  Else make a new subcloud.
        if (ip_address_info.cloud_id == cloud.id) {
            Cloud& subcloud = new_cloud(cloud, ip_address_info.as_name);
            ip_address_info.cloud_id = subcloud.id;
        }
    }

    emit(ip_address_info);

    return ip_address_info;
}



//  Make a new cloud attached to an interface.
//
Model::Cloud& Model::new_cloud(const Interface& interface, const std::string& description)
{
    long id = this->next_id++;
    Cloud& cloud = this->clouds[id];
    this->cloud_ids_by_interface_addresses[interface.address] = id;
    cloud.id = id;
    cloud.description = description;
    cloud.interface_id = interface.id;
    cloud.cloud_id = 0;

    emit(cloud);

    return cloud;
}


//  Make a new cloud attached to a parent cloud.
//
Model::Cloud& Model::new_cloud(Cloud& parent, const std::string& description)
{
    long id = this->next_id++;
    Cloud& cloud = this->clouds[id];
    cloud.id = id;
    cloud.description = description;
    cloud.interface_id = 0;
    cloud.cloud_id = parent.id;
    parent.child_cloud_ids.insert(id);

    emit(cloud);

    return cloud;
}


//  Reassign all interfaces connected on network B to network A.
//  Delete network B.
void Model::merge_networks(long a_id, long b_id)
{
    for (long interface_id : this->networks[b_id].interfaces) {
        MacAddress address = this->interfaces_by_id[interface_id];
        Interface& interface = this->interfaces_by_address[address];
        interface.network_id = a_id;
        this->networks[a_id].interfaces.insert(interface.id);
        emit(interface);
    }

    Network& b_network = this->networks[b_id];
    b_network.interfaces.clear();
    emit(b_network, true);

    this->networks.erase(b_id);
}


void Model::emit(const Network& network, bool fini)
{
    Lansnoop::Event event;
    event.set_timestamp(this->now);
    event.set_packet(this->packet_count);
    event.mutable_network()->set_id(network.id);
    event.mutable_network()->set_fini(fini);
    std::cout << event;
    std::cout << std::flush;
}


void Model::emit(const Interface& interface, bool fini)
{
    Lansnoop::Event event;
    event.set_timestamp(this->now);
    event.set_packet(this->packet_count);
    event.mutable_interface()->set_id(interface.id);
    event.mutable_interface()->set_fini(fini);
    event.mutable_interface()->set_network_id(interface.network_id);
    event.mutable_interface()->set_address(std::string(interface.address.begin(), interface.address.end()));
    event.mutable_interface()->set_maker(interface.maker);
    std::cout << event;
    std::cout << std::flush;
}


void Model::emit(const IPAddressInfo& ipaddress, bool fini)
{
    Lansnoop::Event event;
    event.set_timestamp(this->now);
    event.set_packet(this->packet_count);
    event.mutable_ipaddress()->set_id(ipaddress.id);
    event.mutable_ipaddress()->set_fini(fini);
    event.mutable_ipaddress()->set_address(std::string(ipaddress.address.begin(), ipaddress.address.end()));
    if (ipaddress.interface_id)
        event.mutable_ipaddress()->set_interface_id(ipaddress.interface_id);
    else
        event.mutable_ipaddress()->set_cloud_id(ipaddress.cloud_id);
    event.mutable_ipaddress()->set_ns_name(ipaddress.ns_name);
    std::cout << event;
    std::cout << std::flush;
}


void Model::emit_traffic_update()
{
    Lansnoop::Event event;
    event.set_timestamp(this->now);
    event.set_packet(this->packet_count);

    for (const MacAddress& mac_address : this->recent_interface_traffic) {
        const Interface& interface = this->interfaces_by_address.at(mac_address);
        (*event.mutable_traffic()->mutable_interface_packet_counts())[interface.id] = interface.packet_count;
    }
    for (long id : this->recent_cloud_traffic) {
        const Cloud& cloud = this->clouds.at(id);
        (*event.mutable_traffic()->mutable_cloud_packet_counts())[id] = cloud.packet_count;
    }
    for (const IPV4Address& ip : this->recent_ipaddress_traffic) {
        const IPAddressInfo& ipaddressinfo = this->ip_addresses.at(ip);
        (*event.mutable_traffic()->mutable_ipaddress_packet_counts())[ipaddressinfo.id] = ipaddressinfo.packet_count;
    }

    std::cout << event;
    std::cout << std::flush;
}


void Model::emit(const Cloud& cloud, bool fini)
{
    Lansnoop::Event event;
    event.set_timestamp(this->now);
    event.set_packet(this->packet_count);
    event.mutable_cloud()->set_id(cloud.id);
    event.mutable_cloud()->set_fini(fini);
    event.mutable_cloud()->set_description(cloud.description);
    if (cloud.interface_id)
        event.mutable_cloud()->set_interface_id(cloud.interface_id);
    else
        event.mutable_cloud()->set_cloud_id(cloud.cloud_id);
    std::cout << event;
    std::cout << std::flush;
}
