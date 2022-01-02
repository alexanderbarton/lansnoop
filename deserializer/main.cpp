#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <cstring>
#include <stdexcept>
#include <time.h>

#include "event.pb.h"
#include "EventSerialization.hpp"


static void print(const Lansnoop::Network& network)
{
    std::cout << "    " << "id:         " << network.id() << "\n";
    std::cout << "    " << "fini:       " << (network.fini()?"true":"false") << "\n";
}


static void print(const Lansnoop::Interface& interface)
{
    std::cout << "    " << "id:         " << interface.id() << "\n";
    std::cout << "    " << "fini:       " << (interface.fini()?"true":"false") << "\n";
    std::cout << "    " << "address:    ";
    bool first = true;
    for (unsigned char c : interface.address()) {
        if (first)
            first = false;
        else
            std::cout << ":";
        std::cout << std::hex << std::setw(2) << std::setfill('0') << int(c) << std::dec;
    }
    std::cout << "\n";
    std::cout << "    " << "network_id: " << interface.network_id() << "\n";
    std::cout << "    " << "maker:      " << interface.maker() << "\n";
}


static void print(const Lansnoop::IPAddress& ip_address)
{
    std::cout << "    " << "id:         " << ip_address.id() << "\n";
    std::cout << "    " << "fini:       " << (ip_address.fini()?"true":"false") << "\n";
    std::cout << "    " << "address:    ";
    bool first = true;
    for (unsigned char c : ip_address.address()) {
        if (first)
            first = false;
        else
            std::cout << ".";
        std::cout << int(c);
    } //  TODO: format for IPv6 when needed.
    std::cout << "\n";
    switch (ip_address.attached_to_case()) {
        case Lansnoop::IPAddress::kInterfaceId:
            std::cout << "    " << "attached to: interface " << ip_address.interface_id() << "\n";
            break;
        case Lansnoop::IPAddress::kCloudId:
            std::cout << "    " << "attached to: cloud " << ip_address.cloud_id() << "\n";
            break;
        case Lansnoop::IPAddress::ATTACHED_TO_NOT_SET:
            break;
    }
}


static void print(const Lansnoop::Cloud& cloud)
{
    std::cout << "    " << "id:          " << cloud.id() << "\n";
    std::cout << "    " << "fini:        " << (cloud.fini()?"true":"false") << "\n";
    std::cout << "    " << "description: " << cloud.description() << "\n";
    switch (cloud.attached_to_case()) {
        case Lansnoop::Cloud::kInterfaceId:
            std::cout << "    " << "attached to: interface " << cloud.interface_id() << "\n";
            break;
        case Lansnoop::Cloud::kCloudId:
            std::cout << "    " << "attached to: cloud " << cloud.cloud_id() << "\n";
            break;
        case Lansnoop::Cloud::ATTACHED_TO_NOT_SET:
            break;
    }
}


static void print(const Lansnoop::Traffic& traffic)
{
    std::set<long> interface_keys;
    for (const auto& e : traffic.interface_packet_counts())
        interface_keys.insert(e.first);
    std::set<long> cloud_keys;
    for (const auto& e : traffic.cloud_packet_counts())
        cloud_keys.insert(e.first);
    std::set<long> ipaddress_keys;
    for (const auto& e : traffic.ipaddress_packet_counts())
        ipaddress_keys.insert(e.first);

    std::cout << "    " << "interface counts:\n";
    for (long id : interface_keys)
        std::cout << "                " << id << " => " << traffic.interface_packet_counts().at(id) << "\n";
    std::cout << "    " << "cloud counts:\n";
    for (long id : cloud_keys)
        std::cout << "                " << id << " => " << traffic.cloud_packet_counts().at(id) << "\n";
    std::cout << "    " << "ipaddress counts:\n";
    for (long id : ipaddress_keys)
        std::cout << "                " << id << " => " << traffic.ipaddress_packet_counts().at(id) << "\n";
}


static void print(const Lansnoop::Event& event)
{
    switch (event.type_case()) {
        case Lansnoop::Event::kNetwork:
            std::cout << "Network\n";
            break;
        case Lansnoop::Event::kInterface:
            std::cout << "Interface\n";
            break;
        case Lansnoop::Event::kTraffic:
            std::cout << "Traffic\n";
            break;
        case Lansnoop::Event::kIpaddress:
            std::cout << "IPAddress\n";
            break;
        case Lansnoop::Event::kCloud:
            std::cout << "Cloud\n";
            break;
        case Lansnoop::Event::TYPE_NOT_SET:
        // default:
            std::cout << "[bad event]\n";
            break;
    }

    switch (event.type_case()) {
        case Lansnoop::Event::kNetwork:
            print(event.network());
            break;
        case Lansnoop::Event::kInterface:
            print(event.interface());
            break;
        case Lansnoop::Event::kTraffic:
            print(event.traffic());
            break;
        case Lansnoop::Event::kIpaddress:
            print(event.ipaddress());
            break;
        case Lansnoop::Event::kCloud:
            print(event.cloud());
            break;
        case Lansnoop::Event::TYPE_NOT_SET:
        // default:
            break;
    }

    time_t seconds = event.timestamp() / 1000000000L;
    int milliseconds = (event.timestamp() % 1000000000L) / 1000000L;
    char buffer[256];
    struct tm tm;
    gmtime_r(&seconds, &tm);
    strftime(buffer, sizeof(buffer), "%F %T", &tm);
    std::cout << "    timestamp:  " << buffer << "." << std::setw(3) << std::setfill('0') << milliseconds << "\n";
    std::cout << "    packet:     " << event.packet() << "\n";
    std::cout << std::flush; //  TODO: nix
}


static void run(const std::string& path)
{
    std::ifstream in(path, std::ifstream::binary);
    if (!in.good())
        throw std::invalid_argument("unable to open input path");

    int count = 0;
    Lansnoop::Event event;
    while (in >> event) {
        if (count++)
            std::cout << "\n";
        print(event);
    }
}


int main(int argc, char **argv)
{
    int ret = 0;
    try {
        std::string path = "/dev/stdin";

        int i = 1;
        while (i < argc) {
            path = argv[i++];
        }

        run(path);
    }
    catch (const std::exception& e) {
        std::cerr << argv[0] << ": " << e.what() << std::endl;
        ret = 1;
    }
    catch (...) {
        std::cerr << argv[0] << ": unhandled exception" << std::endl;
        ret = 1;
    }
    return ret;
}
