# Usage overview

OpenSAND emulates a satellite network connected to terrestrial entities. Each
entity of this network must run on their own machine or netnamespace and be
interconnected on a specific, ideally separate, subnetwork. Terrestrial
entities are connected to the rest of the network using TAP interfaces. From
the point of view of the network, OpenSAND acts as a black-box between these
TAP.

Configuration of how packets are routed to or from the TAP interface is
outside the scope of the `opensand` binary but tools and examples are provided
to reduce the learning curve and help users to quickly setup and discover
OpenSAND capabilities. See the
[Command-line User Manual](../opensand-network/README.md) for details.

# OpenSAND Deploy

This package offers a web interface that allows you to Configure and Manage
OpenSAND Entities. Note that this only applies to the behaviour of the entities
and not of the whole network routing packets to or from OpenSAND.

Also note that managing your OpenSAND entities through the web interface
requires you to configure SSH access from the web backend to your entities. It
is, however, not mandatory if you only seek to produce configuration files from
the web interface. See the
[backend configuration](doc/usage.md#configuring-the-opensand-configuration-backend)
for details.

More information on how to build or install this tool is available in the
[dedicated installation manual](doc/install.md).

Documentation on how to use the web interface is available in the
[OpenSAND Deploy User Manual](doc/usage.md).
