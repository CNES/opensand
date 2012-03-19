#!/bin/sh
#
# Test MPEG2-TS/ULE encapsulation scheme.
# Launch test with 'make report'.
#
# Author: Didier Barvaux <didier.barvaux@b2i-toulouse.com>
#

DIRS="icmp_28 icmp_64"

for dir in $DIRS ; do

	echo "Testing $dir..."

	TIME="`/usr/bin/time -f "%E" ./test -o1 $dir/mpeg.pcap -o2 $dir/ip.pcap $dir/source.pcap 2>&1 >/dev/null`"
	echo -e "\ttime = $TIME"

	echo -ne "\tMPEG2-TS frames = "
	md5_mpeg_ref="`md5sum $dir/mpeg_ref.pcap | gawk '{print \$1}'`"
	md5_mpeg="`md5sum $dir/mpeg.pcap | gawk '{print \$1}'`"
	if [ "$md5_mpeg" != "$md5_mpeg_ref" ] ; then
		echo "BAD"
	else
		echo "OK"
	fi

	echo -ne "\tIP packets = "
	md5_ip_ref="`md5sum $dir/ip_ref.pcap | gawk '{print \$1}'`"
	md5_ip="`md5sum $dir/ip.pcap | gawk '{print \$1}'`"
	if [ "$md5_ip" != "$md5_ip_ref" ] ; then
		echo "BAD"
	else
		echo "OK"
	fi

done

