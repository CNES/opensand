#!/bin/bash
#
# Test that configuration files are correct
# Launch test with 'make check'.
#
# Author: Didier Barvaux <didier.barvaux@b2i-toulouse.com>
#

FILES="core_sat core_gw core_st core_global topology"

for file in $FILES ; do

	echo "Testing $file..."

    ./check_xml.py ${file}.conf xsd/${file}.xsd || exit 1
	
done


