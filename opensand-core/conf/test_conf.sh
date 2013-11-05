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

FILES="core_sat core_gw core_st core_global topology"

for file in $FILES ; do
	echo "Testing $file..."
    ${APP} ${BASEDIR}/${file}.conf ${BASEDIR}/xsd/${file}.xsd || exit 1
	
done

echo "Testing mandatory plugins"
for file in `ls ${BASEDIR}/mandatory_plugins`; do
    case $file in
        *.conf)
            name=`echo ${file} | sed 's/.conf$//g'`
            echo "Testing $name..."
            ${APP} ${BASEDIR}/mandatory_plugins/${name}.conf ${BASEDIR}/mandatory_plugins/${name}.xsd || exit 1
        ;;
    esac
done
