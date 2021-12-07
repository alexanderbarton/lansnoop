#include <ostream>
#include <iomanip>
#include "Model.hpp"
#include "event.pb.h"
#include "EventSerialization.hpp"



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
            o << "        network id: " << interface.network_id << "\n";
        }
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
    Interface interface;
    interface.address = address;
    interface.id = this->next_id++;
    interface.network_id = network_id;
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
