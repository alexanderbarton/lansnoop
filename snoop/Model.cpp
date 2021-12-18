#include <ostream>
#include <fstream>
#include <iomanip>
#include <stdexcept>
#include "Model.hpp"
#include "event.pb.h"
#include "EventSerialization.hpp"

#include "/home/abarton/debug.hpp"


void Model::note_l2_packet_traffic(const std::array<unsigned char, 6>& source_address,
                                   const std::array<unsigned char, 6>& destination_address)
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
            long network_id = this->new_network();
            this->new_interface(source_address, network_id);
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
            long network = this->new_network();
            source_i = this->new_interface(source_address, network);
            destination_i = this->new_interface(destination_address, network);
        }
        //  Only the source interface is new.
        else if (source_i == this->interfaces_by_address.end()) {
            source_i = this->new_interface(source_address, destination_i->second.network_id);
        }
        //  Only the destination interface is new.
        else {
            destination_i = this->new_interface(destination_address, source_i->second.network_id);
        }

        //  Note traffic between interfaces.
        //
        //  TODO
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
}


void Model::load_oui(const std::string& path)
{
    //  This seems to be an ISO-4180 formatted file.

    std::ifstream in(path);
    if (!in.good())
        throw std::invalid_argument("unable to read OUI CSV file");

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
std::map<Model::MacAddress, Model::Interface>::iterator Model::new_interface(const MacAddress& address, long network_id)
{
    int oui = (int(address[0]) << 16) | (int(address[1]) << 8) | int(address[2]);
    show(this->ouis.size());
    showx(oui);
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
}


std::ostream& operator<<(std::ostream& o, Model::MacAddress address)
{
    bool first = true;
    for (unsigned char c : address) {
        if (first)
            first = false;
        else
            o << ":";
        o << std::hex << std::setw(2) << std::setfill('0') << int(c) << std::dec;
    }
    return o;
}
