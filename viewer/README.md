# Viewer

This program renders network activity reported by `snoop` as an interactive 3D network graph
with RTS-style navigation.

# Graph Interpretation

The orange boxes are hosts directly connected to the local LAN.
The purple cylinders are IP addresses of hosts that we can't see directly but know exist
because we see IP traffic for them.
The green flashing links between the boxes represent Ethernet packet traffic.

But that's not quite true.

Each orange box really represents a distinct network interface's MAC address for which `snoop` has seen packet traffic.
Most hosts have just one interface; so it's easy to conflate interfaces with their hosts.
An example where it does matter is my laptop.
It's got a wireless interface.
When plugged into its docking station, it's also got a wired interface.
It's also running a VM set up in bridged mode which presents its own virtual interface to the network.

The flickering green links between interfaces represents packet traffic addressed to those interface.
For broadcast Ehternet traffic flickering links show traffic from the sender, but not the
receviers, since those should be all other interfaces.
The links don't show the actual physical path through the network.
E.g., they don't show any intervening switches.

If `snoop` is sniffing packets from the interface of one of these hosts, it's only going to see
traffic that the Etherent switch(s) in the network route to the host.
It's probably _not_ going to see unicast traffic between other hosts.
Installing taps or setting up span ports on routers can give a more complete picture.

The purple cylinders represent a unicast IP address we've seen traffic for.
In some cases (e.g., ARPs) `snoop` can deduce that an IP address is assigned to an interface.
In the absense of those cases, when `snoop` sees IP traffic passing through an Ethernet interface
all it knows is that the IP address is reachable through that interface.
For normal hosts it's a good assumption that a single IP address routed through an interface _is_ that
host's IP address.  But that's not certain.
A gateway router (routing to the internet), or even a host hosting a VM, receives IP traffic for IP addresses
which are not assigned to it.

# Dependencies

sudo apt install libglfw3 libglfw3-dev libglfw3-doc
sudo apt install libxi-dev
sudo apt install libglm-dev

# GLAD
Generate ZIP file at via http://glad.dav1d.de/ .  Then, unpack it in this directory.
It should produce files:
    src/glad.c
    include/glad/glad.h
    include/KHR/khrplatform.h
Also, `touch include/glad/glad.h include/KHR/khrplatform.h src/glad.c` if unzip creates the files with unreasonable timestamps.

# Running

`$ make && sudo ../snoop/build/snoop -v -i enp6s0 --oui ../oui.csv | build/viewer /dev/stdin`

# References
GLFW documentation at https://www.glfw.org/documentation.html .
Also, if libglfw3-doc is installed, file:///usr/share/doc/libglfw3-dev/html/index.html .
