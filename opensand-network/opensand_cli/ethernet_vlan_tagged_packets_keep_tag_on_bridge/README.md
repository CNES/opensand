## Pre-requisite

As with OpenSAND CLI, 'iproute2' and 'make' packages are required. You can install them from your command line terminal by issuing
```bash
apt-get install iproute2 make
```

## Goal of this configuration

This configuration allows to deploy OpenSAND with an IP configuration. No VLAN is present in this configuration, and no QoS can be done in this case.

## Example configuration

We assume the configuration we want is the following. It represents the default configuration of the Makefiles. The parameters can be changed in each Makefile to be adapted to the topology.

![Topology](/opensand-network/opensand_cli/ip/ip_without_vlan.png)

## Usage

All the files in each subfolder (GW, SAT, ST, GW_WS, ST_WS) have to be copied to the corresponding machine.

Once this is done, you have to run Makefiles as root from your command line terminal. Makefiles for WS can be done at any moment, but for OpenSAND entities this order in mandatory: SAT, GW, ST.

The Makefile useful targets are:

- ```make network```: configure the network
- ```make run```: configure the network and launch opensand agent (does not exist on WS)
- ```make stop```: stop opensand agent (does not exist on WS)
- ```make clean```: remove all changes made by network target, and clean network configuration
- ```make```: execute clean, network and run

## Testing

To test the good behavior, with the example topology we chose, we can ping WS_ST from WS_GW:

```bash
ping 192.168.2.2
```

The ping should receive response, with a delay of around 500ms

## Now, you can enjoy it! 