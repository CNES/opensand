## Pre-requisite

'iproute2' and 'make' packages are required. You can install them from your command line terminal by issuing
```bash
apt-get install iproute2 make
```

To do QoS with IP configuration, the module ```br_netfilter``` must be enabled on GW and ST:

```bash
modprobe br_netfilter
```

## Goal of this configuration

This configuration allows to deploy OpenSAND with an IP configuration. A default VLAN is present in this configuration, allowing to perform QoS on OpenSAND. Ethernet frames on OpenSAND are VLAN tagged frames (Ethertype = 0x8100, IEEE 802.1Q).

## Example configuration

We assume the configuration we want is the following. It represents the default configuration of the Makefiles. The parameters can be changed in each Makefile to be adapted to the topology.

![Topology](/opensand-network/opensand_cli/ip_vlan/ip_with_vlan.png)

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

Quality of Service is done in the GW (forward link) and ST (return link) nodes. A YAML file is given for both GW and ST, to perform a mapping between some IP parameters of the packets and the VLAN priority (field PCP of 802.1Q header).

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

To test the good behavior, with the example topology we chose, we can ping WS_ST from WS_GW:

```bash
ping 192.168.2.2
```

The ping should receive response, with a delay of around 500ms.

The ping must also pass between GW and ST directly, still with 500ms delay. For example on GW:

```bash
ping 192.168.63.15
```

To test that the PCP field in correctly set, using default configuration, launch ping with some ToS values from WS_GW:

```bash
ping 192.168.2.2 -Q 224
```

The ToS=224 is mapped by default to PCP=7 in the YAML provided. Then on GW or ST, by entering the following command:

```bash
tcpdump -i opensand_tap -nn -e -v
```

We must see ```vlan 2, p 7```, meaning the default VLAN is used (if the Makefile has not been edited), with a priority of 7.

You can try other mapping and check with ```tcpdump``` if the result is still what you expect.

## Now, you can enjoy it! 