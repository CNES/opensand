# OpenSAND CLI

This piece of software allow to simply deploy OpenSAND on all the agents, and to configure workstations.

Technically, commands are grouped within makefiles and organized into Phony targets or recipes to be executed. Where a recipe consists of one or more actions to carry out. The user has only to edit variable values in the Makefiles to adapt to his topology, and YAML files to customize QoS.

It provides for six modes:

1. ```IP network without VLAN```
2. ```IP network with one default VLAN```
3. ```Ethernet network without VLAN```: incoming packets on GW and ST are default IPv4 trames (0x0800).
4. ```Ethernet network with one default VLAN```: incoming packets on GW and ST are default IPv4 trames (0x0800).
5. ```Ethernet network with several VLAN, cannot ping bridge interface```: incoming packets on GW and ST are 802.1Q trames (0x8100), tagged with VLAN ID and priority.
6. ```Ethernet network with several VLAN, can ping bridge interface```: incoming packets on GW and ST are 802.1Q trames (0x8100), tagged with VLAN ID and priority.

For each configuration, Makefiles are provided for GW, SAT, ST, GW_WS and ST_WS.

Each configuration is described in the README of subfolders. Only configurations 2, 5 and 6 allow to handle QoS on OpenSAND.

## Pre-requisite

'iproute2' and 'make' packages are required. You can install them from your command line terminal by issuing
```bash
apt-get install iproute2 make
```

To do QoS with IP configuration (configuration 2), the module ```br_netfilter``` must be enabled on GW and ST:

```bash
modprobe br_netfilter
```

## Now, you can enjoy it! 