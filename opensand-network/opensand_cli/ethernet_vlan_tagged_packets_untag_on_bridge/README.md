## Pre-requisite

'iproute2' and 'make' packages are required. You can install them from your command line terminal by issuing
```bash
apt-get install iproute2 make
```

## Goal of this configuration

This configuration allows to deploy OpenSAND with an Ethernet 802.1Q configuration. The used can specify several VLANs that are in use between workstations. Thus, packets entering GW or ST are already tagged with a 802.1Q header, and a PCP priority. A default VLAN is present in this configuration, for packets entering OpenSAND without 802.1Q header; in this case the lowest priority is assigned (pcp=0). Ethernet frames on OpenSAND are VLAN tagged frames (Ethertype = 0x8100, IEEE 802.1Q).

This configuration is a simplified version of ```ethernet_vlan_tagged_packets_keep_tag_on_bridge```, thus it is not possible to ping between GW and ST on custom VLANs (only default VLAN can be used to ping).

## Example configuration

We assume the configuration we want is the following. It represents the default configuration of the Makefiles. The parameters can be changed in each Makefile to be adapted to the topology.

![Topology](/opensand-network/opensand_cli/ethernet_vlan_tagged_packets_untag_on_bridge/ethernet_with_vlan_tagged_packets_untag_on_bridge.png)

## Usage

All the files in each subfolder (GW, SAT, ST, GW_WS, ST_WS) have to be copied to the corresponding machine.

Once this is done, you have to run Makefiles as root from your command line terminal. Makefiles for WS can be done at any moment, but for OpenSAND entities this order in mandatory: SAT, GW, ST.

The Makefile useful targets are:

- ```make network```: configure the network
- ```make run```: configure the network and launch opensand agent (does not exist on WS)
- ```make stop```: stop opensand agent (does not exist on WS)
- ```make clean```: remove all changes made by network target, and clean network configuration
- ```make```: execute clean, network and run

## Setting QoS

Quality of Service is done in the WS_GW (forward link) and WS_ST (return link) nodes. A YAML file is given for both WS_GW and WS_ST, to perform a mapping between some IP parameters of the packets and the VLAN priority (field PCP of 802.1Q header).

The IP fields that can be mapped are:

- ```dscp```: dscp
- ```tos```: tos
- ```saddr```: ip_source
- ```daddr```: ip_destination
- ```sport```: port_source (if TCP or UDP transport protocol)
- ```dport```: port_destination (if TCP or UDP transport protocol)
- ```icmp_type```: icmp_type (if ICMP)

For example, if we use the following configuration file:

```yaml
0:
    - dscp: 0
      saddr: "192.168.1.0/24"
1:
    - dscp: 8
      dport: 5000
    - dscp: 10
2:
    - dscp: 16
    - dscp: 18
```

Frames will have priority 0 if:

- ```dscp == 0``` and ```source address == 192.168.1.0/24```

Frames will have priority 1 if:

- ```dscp == 8``` and ```destination port == 5000```
- OR ```dscp == 10```

Frames will have priority 2 if:

- ```dscp == 16```
- OR ```dscp == 18```

## Testing

In the default Makefile provided, we define:

- Default VLAN for packets entering OpenSAND and not tagged: 2
- VLANs to be used between workstations: 10 and 20. Packets enter OpenSAND with the 802.1Q, this field is not altered on GW and ST.

To test the good behavior, with the example topology we chose, we can ping WS_ST from WS_GW (for default and custom VLANs):

```bash
ping 192.168.1.3
ping 192.168.10.3
ping 192.168.20.3
```

The ping should receive response, with a delay of around 500ms.

The ping must also pass between GW and ST directly, but  ONLY ON DEFAULT VLAN, still with 500ms delay. For example on GW:

```bash
ping 192.168.63.15
```

To test that the PCP field in correctly set, using default configuration, launch tcpdump on GW or ST, and look at VLAN ID and associated priotity:

```bash
tcpdump -i opensand_tap -nn -e -v
```

On WS_GW, by default, we have the following mapping:

- dscp = 56 (or tos = 224) -> pcp = 7
- dscp = 40 (or tos = 160) -> pcp = 5
- dscp = 50 (or tos = 200) -> no mapping associated so pcp = 0

You can try several configurations:

- ```ping 192.168.10.3 -Q 224``` -> ```vlan 10, p 7```
- ```ping 192.168.10.3 -Q 160``` -> ```vlan 10, p 5```
- ```ping 192.168.10.3 -Q 200``` -> ```vlan 10, p 0```
- ```ping 192.168.20.3 -Q 224``` -> ```vlan 20, p 7```
- ```ping 192.168.1.3 -Q 224``` -> ```vlan 2, p 0``` (default VLAN, always using lowest priority)

You can try other mapping and check with ```tcpdump``` if the result is still what you expect.

## Now, you can enjoy it!