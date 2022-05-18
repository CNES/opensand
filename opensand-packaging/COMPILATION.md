# Advanced Compilation Manual

This page describes how to manually compile the OpenSAND source code.

The source code is available in the [Net4Sat GitHub OpenSAND project](https://github.com/CNES/opensand).

These procedures are based on the release version `6.0.1`. 

> The compilation checking command `make check` is optional but recommended.

## Output

The Output component provides an API to send probes and logs to the Collector.

Its source code is stored in the `opensand-output` directory.

To compile and install it, execute the following commands:

```
$ cd opensand-output
$ ./autogen.sh
$ make
$ make check
$ sudo make install
```

## Configuration

The Configuration component provides an API to read the OpenSAND configuration files.

Its source code is stored in the `opensand-conf` directory.

This component compilation requires the installation of the Output library:

  * from source
  * or from packages: `libopensand-output` `libopensand-output-dev`

To compile and install it, execute the following commands:

```
$ cd opensand-conf
$ ./autogen.sh
$ make
$ make check
$ sudo make install
```

## Deploy

The deploy component provides tools to generate XML configuration files for OpenSAND entities;
as well as allowing you to launch and monitor them.

Its source code is stored in the `opensand-deploy` directory.

This component compilation requires the installation of the Configuration library:

  * from source
  * or from packages: `libopensand-conf` `libopensand-conf-dev`

To compile and install it, execute the following commands:

```
$ cd opensand-deploy/src/frontend
$ echo "REACT_APP_OPENSAND_VERSION=Manually compiled" > .env
$ yarn install
$ yarn build
```

Detailled installation instructions can be found in the [dedicated installation manual](../opensand-deploy/doc/install.md).

## Real Time

The Real Time component provides real time threads and channels to exchange data between them.

Its source code is stored in the `opensand-rt` directory.

This component compilation requires the installation of the Output library:

  * from source
  * from packages: `libopensand-output` `libopensand-output-dev`

To compile and install it, execute the following commands:

```
$ cd opensand-rt
$ ./autogen.sh
$ make
$ make check
$ sudo make install
```
  
## Core

The Core component provides the binaries that emulate a ST, a GW or a SAT and provides the structure to interface with external libraries.

Its source code is stored in the `opensand-core` directory.

This component compilation requires the installation of the Real Time, Configuration and Output libraries:

  * from source
  * or from packages: `libopensand-output` `libopensand-output-dev` `libopensand-conf` `libopensand-conf-dev``libopensand-rt` `libopensand-rt-dev`

To compile and install it, execute the following commands:

```
  $ cd opensand-core
  $ ./autogen.sh
  $ make
  $ make check
  $ sudo make install
```

## GSE plugin

The GSE plugin interface the external GSE encapsulation library.

Its source code is stored in the `opensand-plugins/encapsulation/gse` directory.

This component compilation requires the installation of the Plugin, Configuration and Output libraries:

  * from source
  * from packages: `libopensand-output` `libopensand-output-dev` `libopensand-conf` `libopensand-conf-dev``libopensand-plugin` `libopensand-plugin-dev`

It also requires the installation of the external GSE encapsulation library:

  * from [source](https://github.com/CNES/libgse)
  * or from packages: `libgse` `libgse-dev`

To compile and install it, execute the following commands:

```
$ cd opensand-plugins/encapsulation/gse
$ ./autogen.sh
$ make
$ make check
$ sudo make install
```

## RLE plugin

The RLE plugin interface the external RLE encapsulation library.

Its source code is stored in the `opensand-plugins/encapsulation/rle` directory.

This component compilation requires the installation of the Plugin, Configuration and Output libraries:

  * from source
  * or from packages: `libopensand-output` `libopensand-output-dev` `libopensand-conf` `libopensand-conf-dev``libopensand-plugin` `libopensand-plugin-dev`

It also requires the installation of the external RLE encapsulation library:

  * from [source](https://github.com/CNES/librle)
  * or from packages: `librle` `librle-dev`

To compile and install it, execute the following commands:

```
$ cd opensand-plugins/encapsulation/rle
$ ./autogen.sh
$ make
$ make check
$ sudo make install
```
