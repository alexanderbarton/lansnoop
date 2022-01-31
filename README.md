# lansnoop

Lansnoop is a tool that graphically displays realtime network activity based on passive observation of packet traffic.

Its current state is that of an unfinished personal recreational coding project.

# Building and Running
## Dependencies
```
sudo apt install -y protobuf-compiler
sudo apt install -y libpcap-dev
sudo apt install -y libglfw3 libglfw3-dev libglfw3-doc
sudo apt install -y libxi-dev
sudo apt install -y libglm-dev
sudo apt install -y libfreetype6-dev
```
## Download Data Files

Obtain the OUI file that maps MAC addresses to manufacturers:
```
wget http://standards-oui.ieee.org/oui/oui.csv
```

IPv4 prefixes and their origin ASNs:
```
wget https://thyme.apnic.net/current/data-raw-table
```
ASN to name mapping for ASNs visible on the Internet today:
```
wget https://thyme.apnic.net/current/data-used-autnums
```

## Build It
```
make depend
make
```

## Run It
The following example assumes your listening interface is `eth0`.  Adjust as necessary.
```
sudo snoop/build/snoop -v -i eth0 --oui oui.csv --prefix data-raw-table --asn data-used-autnums | viewer/build/viewer /dev/stdin
```

# Executables
snoop: Captures packets, interprets network activity, and writes a stream of binary network events to stdout.

deserializer: Converts a stream of binary network events to human-readable text.

viewer: Reads a stream of binary network events and renders a live 3D graph.

# Recepies

Display live traffic:
```
    sudo snoop/build/snoop -v -i eth0 --oui oui.csv --asn data-used-autnums --prefix data-raw-table | viewer/build/viewer /dev/stdin
```

Do it again, but in multiple steps and saving intermediate data to files:
```
    sudo tcpdump -i eth0 -w eth0.pcap
    snoop/build/snoop -v -r eth0.pcap --oui oui.csv --asn data-used-autnums --prefix data-raw-table > eth0.events
    deserialize/build/deserialize eth0.events   # Optionally view the events.
    viewer/build/viewer eth0.events
```

# Credits

Sean Barrett's stb_image.h came from https://github.com/nothings/stb/blob/master/stb_image.h

Much of the OpenGL code was pasted from learnopengl.com.
