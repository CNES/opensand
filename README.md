OpenSAND is an user-friendly and efficient tool to emulate satellite
communication systems, mainly DVB-RCS - DVB-S2.

It provides a suitable and simple means for performance evaluation and
innovative access and network techniques validation. Its ability to interconnect
real equipments with real applications provides excellent demonstration means.

The source codes of OpenSAND components is distributed "as is" under the terms
and conditions of the GNU GPLv3 license or the GNU LGPLv3 license.

Visit us at [opensand.org](https://www.opensand.org/).

# OpenSAND v6.1.0

## Manuals

 * [Installation Manual](README.md#installation-manual)
 * [Compilation Manual](opensand-packaging/README.md)
 * [Command-line User Manual](opensand-network/README.md)
 * [Web-interface User Manual](opensand-deploy/README.md)

# Installation Manual

This page describes how to install a simple OpenSAND platform, containing one satellite,
one gateway (GW), and one Satellite terminal (ST).

## Requirements

### Architecture

In order to deploy a platform, a minimum of three machines must be available:

- one for the satellite,
- one for the gateway,
- and one for the satellite terminal.

They must be connected as shown in the image below, with the following networks:

- one network for the emulated satellite link (`EMU`)
- one network for the workstations connected to the GW (`LAN-GW0`)
- one network for the workstations connected to the ST (`LAN-ST1`)

![Installation schema](/schema_install.png)

In the image, two additional machines (`WS-ST1` and `WS-GW0`) are shown but are actually not
necessary; traffic can be exchanged between the ST1 and GW0 without the need of workstations.
However, the interfaces for the networks `LAN-ST1` and `LAN-GW0` must exist, even if no other
computers are connected to it.

### Operating System

The testbed was tested using Ubuntu 22.04 LTS, but it should work on every Linux distribution
or Unix-like system provided that the required dependencies are installed. However, if you
are not on a Debian-based system, you may need to [compile OpenSAND](opensand-packaging/README.md)
yourself.

## Install

This manual describes how to obtain and install OpenSAND using the distributed debian
packages. For more information about how to obtain OpenSAND sources, compile it, and
install it, please refer to the [compilation manual](opensand-packaging/README.md).

The debian packages can be installed using `apt` commands.

Unless otherwise specified, all the commands here must be executed on all machines.

In order to install the packages using `apt`, the repository must be added to its sources. One way of doing it is by creating the file `/etc/apt/sources.list.d/net4sat.list`.

Start by adding the GPG key for the Net4Sat repository:

```
sudo mkdir /etc/apt/keyrings
curl -sS https://raw.githubusercontent.com/CNES/net4sat-packages/master/gpg/net4sat.gpg.key | gpg --dearmor | sudo dd of=/etc/apt/keyrings/net4sat.gpg
```

Then add the repository and link it to the aforementionned GPG key:

```
cat << EOF | sudo tee /etc/apt/sources.list.d/github.net4sat.sources
Types: deb
URIs: https://raw.githubusercontent.com/CNES/net4sat-packages/master/jammy/
Suites: jammy
Components: stable
Signed-By: /etc/apt/keyrings/net4sat.gpg
EOF
```

An apt sources update is necessary after adding the repository:

```
sudo apt update
```

Next, the packages must be installed. In this manual, the OpenSAND collector are installed on
the machine with the satellite component, but they can be installed anywhere, even on a machine
without opensand. The only constraint is that all the machines must be connected to a same network.

The OpenSAND core, with all its dependencies, is distributed with the meta-package `opensand`; the
OpenSAND collector with `opensand-collector`; and the OpenSAND configuration tool with `opensand-deploy`.

To install the OpenSAND core on the ST, GW and SAT, execute:

```
sudo apt install opensand
```

### Optional configuration

To install the OpenSAND configuration tool, execute:

```
sudo apt install opensand-deploy
```

The configuration tool is a web server and frontend allowing you to create configuration files
for your OpenSAND entities and remotely deploy them to launch OpenSAND binaries. More information
is available in the [dedicated documentation](opensand-deploy/README.md).

### Optional collector

Before installing the collector, some packages need to be installed first:

- Kibana v6.2.4
- Chronograf v1.7.3
- Logstash v6.2.4
- ElacticSearch v6.2.4
- InfluxDB v1.7.1

Logstash needs to have Java installed. JDK version 8 is recommended for this version of logstash, and can be installed via:

```
sudo apt install openjdk-8-jdk
```

To do so, execute the following commands to download the packages:

```
wget https://artifacts.elastic.co/downloads/kibana/kibana-6.2.4-amd64.deb
wget https://dl.influxdata.com/chronograf/releases/chronograf_1.7.3_amd64.deb
wget https://artifacts.elastic.co/downloads/logstash/logstash-6.2.4.deb
wget https://artifacts.elastic.co/downloads/elasticsearch/elasticsearch-6.2.4.deb
wget https://dl.influxdata.com/influxdb/releases/influxdb_1.7.1_amd64.deb
```

Then install the downloaded packages:

```
sudo dpkg -i kibana-6.2.4-amd64.deb
sudo dpkg -i chronograf_1.7.3_amd64.deb
sudo dpkg -i logstash-6.2.4.deb
sudo dpkg -i elasticsearch-6.2.4.deb
sudo dpkg -i influxdb_1.7.1_amd64.deb
```

To install the OpenSAND collector, execute:

```
sudo apt install opensand-collector
```

> :warning: **If you are using OpenSAND through the OpenBach orchestrator, do **not** install this
collector on the same machine than the OpenBach collector. Even though the tools installed are the
same and used similarly, they have a mutually exclusive configuration. Installing one will break
the other.**

## Uninstall

In order to uninstall OpenSAND, you can remove every OpenSAND package easily by executing:

```
sudo apt remove --purge opensand* libopensand* librle libgse
```

Be careful not to execute the command while on an OpenSAND folder (uninstall may try to remove said folder).
