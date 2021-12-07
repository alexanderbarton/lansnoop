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
        case Lansnoop::Event::TYPE_NOT_SET:
        default:
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
        case Lansnoop::Event::TYPE_NOT_SET:
        default:
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
