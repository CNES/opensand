# Easy OpenSAND CLI

This is a small piece of software allowing to make the setup of a minimal OpenSAND platform easier. We invite you to follow this step-by-step approach. 
Unlike 'OpenSAND CLI' where you have to manualy run each command, 'Easy OpenSAND CLI' groups commands by steps: configure/clean network and run/stop opensand binaries.
Technically, commands are grouped within makefiles and organized into Phony targets or recipes to be executed. Where a recipe consists of one or more actions to carry out.

It provides for six modes:

1. IP network without VLAN
2. IP network with one default VLAN
3. Ethernet network without VLAN
4. Ethernet network with one default VLAN
5. Ethernet network with several VLAN, cannot ping bridge interface
6. Ethernet network with several VLAN, can ping bridge interface

For each configuration, Makefiles are provided for GW, SAT, ST, GW_WS and ST_WS.

Each configuration is described in the README of subfolders. Only configurations 2, 5 and 6 allow to handle QoS on OpenSAND.

## Pre-requisite

As with OpenSAND CLI, 'iproute2' and 'make' packages are required. You can install them from your command line terminal by issuing
```bash
apt-get install iproute2 make
```

## Now, you can enjoy it! 