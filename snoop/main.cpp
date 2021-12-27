#include <iostream>
#include <string>
#include <cstring>
#include <stdexcept>

#include <signal.h>
#include <pcap.h>

#include "EventSerialization.hpp"
#include "Snoop.hpp"


static pcap_t* global_libpcap = nullptr;


extern "C" {
    static void pcap_callback(u_char *user, const struct pcap_pkthdr *hdr, const u_char *frame)
    {
        reinterpret_cast<Snoop*>(user)->parse_ethernet(hdr->ts, frame, hdr->caplen);
    }
}


static void signal_handler(int sig, siginfo_t*, void*)
{
    std::cerr << "\nCaught " << strsignal(sig) << " signal" << std::endl;

    /*  Restore default signal handling.
     */
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);

    /*  Ask libpcap to quit.
     */
    if (global_libpcap)
        pcap_breakloop(global_libpcap);
}


static void register_signal_handler()
{
    struct sigaction sa;
    sa.sa_sigaction = signal_handler;
    sa.sa_flags = SA_SIGINFO;
    sigemptyset(&sa.sa_mask);
    sigaction(SIGHUP, &sa, NULL);
    sigaction(SIGINT, &sa, NULL);
    sigaction(SIGTERM, &sa, NULL);
}


static void capture(pcap_t* libpcap, bool live_capture, Snoop* snoop)
{
    //  Fail if the packets are anyting other than Ethernet.
    //
    switch (pcap_datalink(libpcap)) {
        case DLT_EN10MB:
            // OK.
            break;

        default:
            throw std::invalid_argument(std::string("unexpected link type ") + pcap_datalink_val_to_name(pcap_datalink(libpcap)));
    }

    int ret = 0;
    int idle_count = 0;
    u_char* user_data = reinterpret_cast<u_char*>(snoop);
    for (;;) {
        ret = pcap_dispatch(libpcap, -1, pcap_callback, user_data);

        if (-1 == ret) {
            break; //  Error.  Handle below.
        }
        else if (-2 == ret) {
            break;  //  pcap_breakloop() called.  We're done.
        }
        else if (0 == ret) {
            //  Check for EOF when reading from pcap file.
            if (!live_capture)
                break;

            //  Notify the packet parser that time has passed.
            //  If libpcap's idle timeout is 1ms, this will happen
            //  at most every 10ms.
            if (++idle_count > 10) {
                struct timeval now;
                gettimeofday(&now, NULL);
                snoop->parse_ethernet(now, NULL, 0);
                idle_count = 0;
            }
        }
        else {
            //  pcap_dispatch() processed one or more packets.
            idle_count = 0;
        }
    }

    if (-1 == ret) {
        auto error = std::runtime_error(std::string("pcap_dispatch(): ") + pcap_geterr(libpcap));
        pcap_close(libpcap);
        libpcap = nullptr;
        throw error;
    }
}


static pcap_t* read_interface(const std::string &nic)
{
    int ret;
    char errbuf[PCAP_ERRBUF_SIZE];
    *errbuf = '\0';

    pcap_t* libpcap = pcap_create(nic.c_str(), errbuf);
    if (!libpcap || *errbuf)
        throw std::runtime_error(errbuf);

    pcap_set_promisc(libpcap, 1);

    //  Set 1 ms timeout.
    ret = pcap_set_timeout(libpcap, 1);
    if (ret) {
        pcap_close(libpcap);
        libpcap = nullptr;
        throw std::runtime_error("pcap_set_timeout() failed");
    }

    //  32MB packet buffer.
    ret = pcap_set_buffer_size(libpcap, 32 * 1024*1024);
    if (ret) {
        pcap_close(libpcap);
        libpcap = nullptr;
        throw std::runtime_error("pcap_set_buffer_size() failed");
    }

    //  1600 byte snaplen.
    ret = pcap_set_snaplen(libpcap, 1600);
    if (ret) {
        pcap_close(libpcap);
        libpcap = nullptr;
        throw std::runtime_error("pcap_set_snaplen() failed");
    }

    if (pcap_activate(libpcap)) {
        auto error = std::runtime_error(std::string("pcap_activate: ") + pcap_geterr(libpcap));
        pcap_close(libpcap);
        libpcap = nullptr;
        throw error;
    }

    return libpcap;
}


static void usage(const char* argv0, std::ostream& out)
{
    out << "Usage: " << argv0 << " [-v] [-i interface] [--oui oui_file] [-r pcap_file]" << std::endl;
    out << "Writes binary network activity to stdout." << std::endl;
    out << std::endl;
    out << "  -i    Read packets from the named interface." << std::endl;
    out << "  --oui Read OUI information from the names CSV file." << std::endl;
    out << "  -r    Read packets from the named libpcap savefile." << std::endl;
    out << "  -v    Be verbose.  Print packet stats to stderr on exit." << std::endl;
}


static pcap_t* read_file(const std::string &path)
{
    char errbuf[PCAP_ERRBUF_SIZE];
    pcap_t* libpcap = pcap_open_offline(path.c_str(), errbuf);
    if (!libpcap)
        throw std::invalid_argument(errbuf);

    return libpcap;
}


int main(int argc, char **argv)
{
    int ret = 0;
    try {

        GOOGLE_PROTOBUF_VERIFY_VERSION;

        std::string file, iface, oui_path;

        bool verbose = false;
        int i = 1;
        while (i < argc) {
            if (std::string("-?") == argv[i] || std::string("--help") == argv[i]) {
                ++i;
                usage(argv[0], std::cout);
            } else if (std::string("-i") == argv[i]) {
                ++i;
                if (i >= argc)
                    throw std::invalid_argument("-i expects an interface name, none given");
                iface = argv[i++];
            }
            else if (std::string("--oui") == argv[i]) {
                ++i;
                if (i >= argc)
                    throw std::invalid_argument("--oui expects a CSV file name, none given");
                oui_path = argv[i++];
            }
            else if (std::string("-r") == argv[i]) {
                ++i;
                if (i >= argc)
                    throw std::invalid_argument("-r expects a file name, none given");
                file = argv[i++];
            }
            else if (std::string("-v") == argv[i]) {
                ++i;
                verbose = true;
            }
            else {
                throw std::invalid_argument(std::string("unexpected argument: ") + argv[i]);
            }
        }

        if (1 != file.empty() + iface.empty())
            throw std::invalid_argument("please provide either a pcap savefile (-r filename) or an interface (-i iface) to read packets from");

        register_signal_handler();

        Snoop snoop;
        if (oui_path.size())
            snoop.get_model().load_oui(oui_path);

        pcap_t* libpcap;
        if (file.size())
            global_libpcap = libpcap = read_file(file);
        else
            global_libpcap = libpcap = read_interface(iface);
        bool live_capture = iface.size();

        capture(libpcap, live_capture, &snoop);
        global_libpcap = nullptr;

        pcap_close(libpcap);

        if (verbose) {
            std::cerr << snoop.get_stats() << "\n";
            std::cerr << "\n";
            snoop.get_model().report(std::cerr);
        }
    }
    catch (const std::exception& e) {
        std::cerr << argv[0] << ": " << e.what() << std::endl;
        ret = 1;
    }
    catch (...) {
        std::cerr << argv[0] << ": unhandled exception" << std::endl;
        ret = 1;
    }
    google::protobuf::ShutdownProtobufLibrary();
    return ret;
}
