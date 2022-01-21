# Dependencies
    sudo apt install -y protobuf-compiler
    sudo apt install -y libpcap-dev
    g++

# Executables
    snoop: Captures packets, interprets network activity, and writes a stream of binary network events to stdout.
    deserializer: Converts a stream of binary network events to human-readable text.
    viewer: Reads a stream of binary network events and renders a live 3D graph.

# Recepies
Obtain the OUI file:
    wget http://standards-oui.ieee.org/oui/oui.csv

Display live traffic:
    sudo snoop/build/snoop -v -i enp6s0 --oui oui.csv --asn ../asndata/data-used-autnums --prefix ../asndata/data-raw-table | build/viewer /dev/stdin

Do it again, but in multiple steps and saving intermediate data files:
    sudo tcpdump -i enp6s0 -w enp6s0.pcap
    snoop/build/snoop -v -r enp6s0.pcap --oui oui.csv > enp6s0.events
    deserialize/build/deserialize enp6s0.events
    viewer/build/viewer enp6s0.events
