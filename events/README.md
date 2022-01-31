# Conventions

 * Objects have a unique ID stored in their `id` field.
 * Object IDs will be unique among all objects.
 * They won't be re-used to identify a different object after the first's object's lifetime is over..
 * Object IDs are always greater than 0.  (0 is reserved to mean "nothing".)
 * Objects refer to other objects via a `foo_id` field.
 * An object is "done" when its `fini` field is set. Once set, the object won't be updated again.  It's safe to delete it.
 * No object will refer to a "fini" object.

# Object Model

A Network represents a layer two network.  (E.g. Ethernet.)
A Network contains Interfaces.

An Interface represents a layer two network interface device.
Each Interface always belongs to exactly one network.
An Interface may directly contain any number IPAddresses.  These addresses are assigned directly to the interface.
An interface may optionally contain up to one Cloud.  That cloud contains IPAddresses reachable through the interface
but not directly assigned ot the interface.

An IPAddress represents everything we know about an IP address.
An IPAddress is always attached to either one Interface or one Cloud.

A Cloud represents a group of IPAddresses.
A Cloud contians IPAddresses and other child Clouds.
Each cloud is contained by either one parent Cloud or one Interface.

Network <- Interface <- Cloud <-|
            ^            ^  |---|
       IPAddress   IPAddress

A network Interface is not a host.  It's possible that a host could be connected to the same network
via two or more interfaces.  (When my laptop is docked, an ICMP ping request arriving at its wireless
interface will be answered through its wired interface.)

A naive every-day model of a network assumes that a host has an IP address and it has a nic which has a MAC address.
And that a host's MAC address and its IP address are synonomous.  That model is actually right most of the time for
typical equipment (like desktop computers).

One defect of this model is that a cloud can be attached to only one interface.
E.g., when we have traffic for the same destination IP address flowing through two gateway routers, 
it's really one Cloud beyond the routers shared by both.  But this model doesn't allow them to share.
