# Compilation Manual

This page describes how to compile the OpenSAND source code, generate the
Debian packages and install it from the source.

The source code is available in the [Net4Sat GitHub OpenSAND project](https://github.com/CNES/opensand).

These procedures are based on the release version `6.1.0`.


## Requirements

The code is compiled regularly on __Ubuntu 20.04 LTS__ and __Ubuntu 22.04 LTS__ with __x64__ processors.

The following table lists the required packages to compile OpenSAND:

| OS | Required packages |
| :---: | :--- |
| __Ubuntu 20.04 LTS__ | build-essential apt-utils debhelper sudo fakeroot software-properties-common autotools-dev automake libtool pkg-config gcc g++ python python3-dev libxml++2.6-dev libboost-python1.71-dev libgoogle-perftools-dev libpcap-dev rsyslog logrotate python-setuptools python3-netifaces bridge-utils python-lxml dpkg-dev nodejs yarn |
| __Ubuntu 22.04 LTS__ | build-essential apt-utils debhelper sudo fakeroot software-properties-common autotools-dev automake libtool pkg-config gcc g++ python3-dev libxml++2.6-dev libboost-python1.74-dev libgoogle-perftools-dev libpcap-dev rsyslog logrotate python-setuptools python3-netifaces bridge-utils python3-lxml dpkg-dev dh-python nodejs yarn |

The versions of NodeJS and Yarn required to be able to compile the `opensand-deploy` package does not match the defaults provided with the OS. You will need to install them from an external repository:

```
$ curl -fsSL https://deb.nodesource.com/setup_14.x | sudo -E bash -
$ curl -sL https://dl.yarnpkg.com/debian/pubkey.gpg | gpg --dearmor | sudo tee /usr/share/keyrings/yarnkey.gpg >/dev/null
$ echo "deb [signed-by=/usr/share/keyrings/yarnkey.gpg] https://dl.yarnpkg.com/debian stable main" | sudo tee /etc/apt/sources.list.d/yarn.list
$ sudo apt-get update
$ sudo apt-get install nodejs yarn
```

When compiling the encapsulation and lan adaptation plugins, the development version of their associated library is also needed.

The procedure to install these libraries is as follows:

```
$ curl -sS https://raw.githubusercontent.com/CNES/net4sat-packages/master/gpg/net4sat.gpg.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/net4sat.gpg >/dev/null
$ echo "deb https://raw.githubusercontent.com/CNES/net4sat-packages/master/ focal stable" | sudo tee /etc/apt/sources.list.d/net4sat.list
$ sudo apt-get update
$ sudo apt-get install librle-dev libgse-dev
```

> Docker and LXC can be used to compile OpenSAND source code.

## Source Code and Package Overview

This section describes the OpenSAND source code and packages that are generated after compilation.

### Source Code Presentation

The OpenSAND source code is divided in several directories. This table lists the most important:

| Directory | Language | Description |
| :-------: | :------: | :---------- |
| `opensand-output` | C++ | Source code of the Output library which provides an API to send probes and logs to the Collector of the testbed |
| `opensand-conf` | C++ & Python 3 | Source code of the Configuration library which provides an API to read the OpenSAND configuration |
| `opensand-rt` | C++ | Source code of the Real Time library which provides real time threads and channels to exchange data between them |
| `opensand-core` | C++ | Source code of the OpenSAND entities which emulate a ST, a GW or a SAT \\ Source code of the Plugin library which implements a structure to interface external libraries |
| `opensand-deploy` | Python 3 & Javascript | Source code of the web server allowing to configure, launch and monitor  OpenSAND entities |
| `opensand-plugins/encapsulation/gse` | C++ | Source code of the GSE plugin which interfaces the external GSE library |
| `opensand-plugins/encapsulation/rle` | C++ | Source code of the RLE plugin which interfaces the external RLE library |

### Generated Packages

Each source code directory is able to generate one or more packages.
They are detailed on the following table:

| Directory | Package | Contents |
| :-------: | :-----: | :------- |
| `opensand-output` | `libopensand-output` | Output library for exploitation |
| ::: | `libopensand-output-dev` | Output library headers for development |
| ::: | `libopensand-output-dbg` | Output library symbols for debug |
| `opensand-conf` | `libopensand-conf` | Configuration library for exploitation |
| ::: | `libopensand-conf-dev` | Configuration library headers for development |
| ::: | `libopensand-conf-dbg` | Configuration library symbols for debug |
| `opensand-rt` | `libopensand-rt` | Real Time library for exploitation |
| ::: | `libopensand-rt-dev` | Real Time library headers for development |
| ::: | `libopensand-rt-dbg` | Real Time library symbols for debug |
| `opensand-core` | `opensand-core-conf` | Configuration files for Core exploitation |
| ::: | `opensand-core` | Core binaries for exploitation |
| ::: | `opensand-core-dbg` | Core binaries symbols for debug |
| ::: | `libopensand-plugin` | Plugin library for exploitation |
| ::: | `libopensand-plugin-dev` | Plugin library headers for development |
| ::: | `libopensand-plugin-dbg` | Plugin library symbols for debug |
| `opensand-deploy` | `opensand-deploy` | Configuration web server files |
| `opensand-plugins/encapsulation/gse` | `libopensand-gse-encap-plugin-conf` | Configuration files of the GSE Encapsulation plugin for exploitation |
| ::: | `libopensand-gse-encap-plugin` | Library the GSE Encapsulation plugin for exploitation |
| ::: | `libopensand-gse-encap-plugin-dbg` | Library symbols the GSE Encapsulation plugin for debug |
| ::: | `libopensand-gse-encap-plugin-manager` | Manager module of the GSE Encapsulation plugin |
| `opensand-plugins/encapsulation/rle` | `libopensand-rle-encap-plugin-conf` | Configuration files of the RLE Encapsulation plugin for exploitation |
| ::: | `libopensand-rle-encap-plugin` | Library the RLE Encapsulation plugin for exploitation |
| ::: | `libopensand-rle-encap-plugin-dbg` | Library symbols the RLE Encapsulation plugin for debug |
| ::: | `libopensand-rle-encap-plugin-manager` | Manager module of the RLE Encapsulation plugin |

Additional meta packages can be generated:

  * `opensand`: installs all packages required by an OpenSAND entity (ST, GW or SAT) with required plugins
  * `opensand-collector`: gather all packages necessary for a collector machine and configure them

## Compile source to generate package

OpenSAND source code is usually compiled to generate Debian packages, to be installed next.

To make this operation easier, a script has been developed to automatize the packaging from
source, including compilation.

This script is present in the directory `opensand-packaging` and is capable of generating
__Ubuntu 20.04 LTS__ and __Ubuntu 22.04 LTS__ packages, but it could be extended to other
distributions.

The help message prompted by this script `build-pkgs` is presented below:

```
$ ./opensand-packaging/build-pkgs -h
Usage : build-pkgs [OPTIONS] -s <src path> -d <dst path> -t <dist> TARGET
Available TARGETS :   all:      build all packages (default)
                      libs:     build only libraries packages
                      core:     build only core packages
                      services: build only services packages
                      encap:    build only encapsulation plugins
                      lan:      build only lan adaptation plugins
Available OPTIONS :   -i        install build-dependencies automatically
                      -f        add a user flag to packages
List of available distributions:
focal
jammy
```

For example, the command to execute in order to launch the complete packaging, assuming the
OpenSAND source code directory is the current directory and the compilation host is __Ubuntu 20.04 LTS__:

```
$ ./opensand-packaging/build-pkgs -s . -d ../workspace -t focal all
```

When the compilation ends, you should find OpenSAND packages in the `../workspace/pkgs` directory,
and the source code used to compile it in the `../workspace/src`.

In case of compilation error, the script should stop and display error location.
To display the complete error message, you can consult the two following log files inside the
specific component directory  (in `src` directory):

  * `config.log`: configuration log file
  * `build.log`: compilation log file

For more information about how to install OpenSAND without generating the debian packages,  consult the [Advanced Compilation Manual](COMPILATION.md) page.
