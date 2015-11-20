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
DIRS=`ls examples`

# Test base configurations
for file in $FILES ; do
	echo "Testing $file..."
    ${APP} ${BASEDIR}/${file}.conf ${BASEDIR}/xsd/${file}.xsd || exit 1
	
done

# Test plugins
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

# Test examples
for dir in ${DIRS}; do
    dir="examples/${dir}"
    echo "Testing GW0 for scenario ${dir}"
    ${APP} ${dir}/gw0/core.conf ${BASEDIR}/xsd/core_gw.xsd || exit 1
    echo "Testing SAT for scenario ${dir}"
    ${APP} ${dir}/sat/core.conf ${BASEDIR}/xsd/core_sat.xsd || exit 1
    echo "Testing ST1 for scenario ${dir}"
    ${APP} ${dir}/st1/core.conf ${BASEDIR}/xsd/core_st.xsd || exit 1
    echo "Testing ST2 for scenario ${dir}"
    ${APP} ${dir}/st2/core.conf ${BASEDIR}/xsd/core_st.xsd || exit 1
    echo "Testing Global for scenario ${dir}"
    ${APP} ${dir}/core_global.conf ${BASEDIR}/xsd/core_global.xsd || exit 1
    echo "Testing Topology for scenario ${dir}"
    ${APP} ${dir}/topology.conf ${BASEDIR}/xsd/topology.xsd || exit 1
done
