# OpenSAND Network

This folder provides tools to help network configuration on OpenSAND entities.
None of these tools are mandatory, their main purpose is to reduce the learning
curve and provide quick and easy ways to discover OpenSAND capabilities.

The `opensand_network` folder provides a Python module to easily create TAP,
bridges and routing on a machine or inside a net-namespace.

The [`opensand_cli`](opensand_cli/README.md) is a collection of Makefiles to
easily configure your network on a machine and run a simple configuration of
OpenSAND once done.

## opensand-network

This script provide an API to easily manage net-namespaces for your OpenSAND
processes. This allows to easily run your entire emulation on a single machine.
It is comprised of 3 main commands:

```
sudo ./opensand-network setup -g 6 -s 0 -t 1 2 3
```

This command will create a net-namespace for a gateway with an ID of 6, setup
a bridge and a tap inside; it will create a second net-namespace for a satellite
with an ID of 0; lastly it will create 3 net-namespace for a terminal, with
respective IDs of 1, 2, and 3, and setup a bridge and a tap inside each of them.
Emulation addresses are of the form 192.168.18.ID and TAP are created with a
name of `opensand_tap` and addresses of the form 192.168.63.ID.

```
sudo ./opensand-network launch gw 6 path/to/topology.xml path/to/infrastructure.xml path/to/profile.xml
sudo ./opensand-network launch sat 0 path/to/topology.xml path/to/infrastructure.xml
sudo ./opensand-network launch st 1 path/to/topology.xml path/to/infrastructure.xml path/to/profile.xml
sudo ./opensand-network launch st 2 path/to/topology.xml path/to/infrastructure.xml path/to/profile.xml
sudo ./opensand-network launch st 3 path/to/topology.xml path/to/infrastructure.xml path/to/profile.xml
```

These commands will each run an OpenSAND process into the designated net-namespaces and wait for
completion. Ideally you want to run them in a dedicated terminal each.

```
sudo ./opensand-network clean
```

This command will remove all the net-namespaces created previously.


> Note that all these commands need to be run as root because they all use the `ip netns exec`
command internally that requires root privileges.

Extra options exists for all these commands, use the `--help` flag to learn about them.

An additional command also help to define networks to deal with ISL communications:

```
sudo ./opensand-network isl 2 9
```

This command create a network usable between the satellites with IDs 2 and 9. It
offers a TAP named `isl_<count>_tap` where `count` is the amount of `isl` commands
ran for _that_ satellite, and addresses of the form 10.10.ID.<count> on each
satellite that are meant to be used in an Interconnect block (see the user manual on
ISL configuration).

### Intégration with `opensand_cli`

You can use the configurations presented in the `opensand_cli` folders and launch OpenSAND through
the `opensand-network` script. To do so, issue the `make generate-xml` command in the `opensand_cli`
folder of your liking and indicate the path to the generated files to the `opensand-network launch`
command.
