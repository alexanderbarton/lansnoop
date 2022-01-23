The OUI list can be downloaded from `http://standards-oui.ieee.org/oui/oui.csv`.

Here's how the model works (currently)
======================================

An Ethernet broadcast packet tells us that the interface exists and we now know its MAC address.

We can (probably) tell the manufacturer of the interface from its address's OUI.

An Enternet packet to/from a pair of interfacess tells us that the two interfaces are on the same logical LAN segment.
They may be separated switches.

Association between interfaces and networks is transitive.  So, if we see Ethernet traffic from an interface
connected to network A, and it's to an interface on network B, then we now know that the two networks
are the same.  The model can them merge the two networks.

An Ethernet packet carrying IP traffic to an interface tells us that the the IP address is reachable through
that interface.  And nothing more.  It's possible that the IP address is assigned to the interface.
It's also possible that the interface is packet forwarding gateway (routing) the IP traffic to another
network.

An ARP reply can tell us that an IP address at the interface.

We can spy on DNS A and PTR records to to associate names to IP addresses.

We can identify IP addresses as RFC-1918 private addresses.  These addresses are probably "inside" our network
behind a NATing router.  Non-private addresses are probably hosts on the internet.

We can group public IP addresses by ASN and display their owners.


Here's other stuff that might could be done
===========================================

Grab IP address names from DHCP and other protocols.

A DHCP-provided netmask could hint that IPs routed to endpoint interfaces actually belong to those endpoints.
(That's just a guess.  Especially if there are multiple NATted networks.)

Could we look at IP routing and determine that a pair of interfaces actually belong to the same host.

We really should associate IP address clouds with multiple parent attachment points.

Could we listen to the router's and switchs' spanning tree protocol stuff and come up with a physical topology?

Show individual ongoing network connections (DNS, HTTP, UDP, TCP, etc.) as threads connecting IP address pairs.

Visually indicate (flash red?) when one of these thrreads is having touble:
- DNS timeouts or slow responses.
- TCP retransmits.
- ICMP errors.
