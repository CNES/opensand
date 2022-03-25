# OpenSAND CLI

This piece of software allow to configure/clean the network topology of your OpenSAND entities (SAT, GWs and STs) and eventually the workstations (WS) but only for QoS purposes (IP-MAC level). It will also allow you to launch/stop the OpenSAND binaries.

Technically, commands are grouped within makefiles and organized into Phony targets or recipes to be executed. Where a recipe consists of one or more actions to carry out. If you are using a simple topology (1 SAT, 1 GW and 1 ST), you have to edit the variable values in the Makefiles to adapt to your topology, and the YAML files to customize QoS (mapping between Ethernet PCP and IP parameters such as DSCP, ToS, ip src/dst, etc.). If your topology is different (e.g. several STs or GWs), you might need to add some commands to the Makefiles.

It provides for six modes:

1. ```IP network without VLAN```: IP configuration with no VLANs and no QoS capable. Ethernet frames on OpenSAND are default IPv4 frames (Ethertype = 0x0800).
2. ```IP network with one default VLAN```: IP configuration with one default VLAN available, allowing to perform QoS on OpenSAND. Ethernet frames on OpenSAND are VLAN tagged frames (Ethertype = 0x8100, IEEE 802.1Q)
3. ```Ethernet network without VLAN```: Ethernet configuration with no VLANs and no QoS capable. Incoming packets on GW and ST are default IPv4 trames (0x0800).
4. ```Ethernet network with one default VLAN```: Ethernet configuration with one default VLAN but no QoS capable. Incoming packets on GW and ST are default IPv4 trames (0x0800).
5. ```Ethernet network with several VLAN, can ping bridge interface```: Ethernet configuration with several VLANs between WSs and allowing to perform QoS. Incoming packets on GW and ST are 802.1Q trames (0x8100), tagged with VLAN ID and priority
6. ```Ethernet network with several VLAN, cannot ping bridge interface```: A simplified version of the configuration where you will not be able to ping GW and ST via custom VLANs (only via the default one).

Here is a table summarizing the main characteristics of each configuration

| Name                                                             | Frame type in LAN networks        | Frame type in OpenSAND network | QoS capable | VLANs in OpenSAND                           |
| :---------:                                                      | :---------:                       | :---------:                    | :---------: | :---------:                                 |
| IP network without VLAN                                          | IPv4 (0x0800)                     | IPv4 (0x0800)                  | No          | None                                        |
| IP network with one default VLAN                                 | IPv4 (0x0800)                     | 802.1Q (0x8100)                | Yes         | 1 default                                   |
| Ethernet network without VLAN                                    | IPv4 (0x0800)                     | IPv4 (0x0800)                  | No          | None                                        |
| Ethernet network with one default VLAN                           | IPv4 (0x0800)                     | 802.1Q (0x8100)                | No          | 1 default                                   |
| Ethernet network with several VLAN, can ping bridge interface    | 802.1Q (0x8100) and IPv4 (0x0800) | 802.1Q (0x8100)                | Yes         | 1 default + several VLANs for tagged frames |
| Ethernet network with several VLAN, cannot ping bridge interface | 802.1Q (0x8100) and IPv4 (0x0800) | 802.1Q (0x8100)                | Yes         | 1 default + several VLANs for tagged frames |

For each configuration, Makefiles are provided for GW, SAT, ST, GW_WS (Workstation connected to the GW LAN) and ST_WS (Workstation connected to the ST LAN).

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
