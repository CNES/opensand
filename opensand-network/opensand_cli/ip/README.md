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

![Topology](/opensand-network/opensand_cli/ip/ip_without_vlan.png)