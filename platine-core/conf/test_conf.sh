#!/bin/bash
#
# Test that configuration files are correct
# Launch test with 'make check'.
#
# Author: Didier Barvaux <didier.barvaux@toulouse.viveris.com>
#

APP="check_xml.py"

# parse arguments
SCRIPT="$0"
BASEDIR=$( dirname "${SCRIPT}" )
APP="${BASEDIR}/${APP}"

FILES="core_sat core_gw core_st core_global"

for file in $FILES ; do
	echo "Testing $file..."
    ${APP} ${BASEDIR}/${file}.conf ${BASEDIR}/xsd/${file}.xsd || exit 1
	
done


