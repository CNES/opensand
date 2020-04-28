# Easy OpenSAND CLI

This is a small piece of software allowing to make the setup of a minimal OpenSAND platform easier. We invite you to follow this step-by-step approach. 
Unlike 'OpenSAND CLI' where you have to manualy run each command, 'Easy OpenSAND CLI' groups commands by steps: configure/clean network and run/stop opensand binaries.
Technically, commands are grouped within makefiles and organized into Phony targets or recipes to be executed. Where a recipe consists of one or more actions to carry out.

It provides for both modes, IP and ETH, makefiles to run on each component: SAT, GW and ST. 

## Pre-requisite

As with OpenSAND CLI, 'iproute2' and 'make' packages are required. You can install them from your command line terminal by issuing
```bash
apt-get install iproute2 make
```

## Usage
It is assumed that the simulation files are already configured and installed on each component, as well as this tool.
You need to modify each makefile to adapt them to your platform by correctly replacing variables value.

Once this is done, you have to run makefiles as root from your command line terminal as follows:
```bash
make network
make run
```
First command configures network interfaces and routing, and second one runs OpenSAND binary.
If needed, you can run 

```bash
make stop
make clean
```
to respectively stop emulation and clean network configurations.

## Now, you can enjoy it! 
.
