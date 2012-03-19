#!/bin/bash
#
# Test encapsulation plugins.
# Launch test with 'make check'.
#
# Author: Didier Barvaux <didier.barvaux@b2i-toulouse.com>
#

DIRS="icmp_28 icmp_64"

for dir in $DIRS ; do

	echo "Testing $dir..."

	./test_plugins -f $dir $dir/source.pcap 1>/dev/null || ./test_plugins -f $dir $dir/source.pcap -d 1
	
done


