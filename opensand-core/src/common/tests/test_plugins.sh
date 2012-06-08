#!/bin/sh
#                                       
# file:        test_plugins.sh
# description: Check the behaviour of the plugin architecture.
# author:      Didier Barvaux <didier.barvaux@toulouse.viveris.com>
# author:      Julien Bernard <julien.bernard@toulouse.viveris.com>
#
# This script may be used by creating a link "test_plugins_DIR.sh"
# where:    
#    DIR     is the path to the capture file that contains the PCAP sources
#            to test the plugins implementation
#
# Script arguments:                     
#    test_plugins_DIR.sh [verbose]
# where:
#   verbose          prints the traces of test application
#
 
# parse arguments                       
SCRIPT="$0" 
VERBOSE="$1"
VERY_VERBOSE="$2"
if [ "x$MAKELEVEL" != "x" ] ; then      
	BASEDIR="${srcdir}"                 
	APP="./test_plugins"
else
	BASEDIR=$( dirname "${SCRIPT}" )    
	APP="${BASEDIR}/test_plugins"
fi

# Extract the directory containing the source captures
INPUT_DIR=$( echo "${SCRIPT}" | \
             sed -e 's#^.*/test_plugins_\(.\+\)\.sh#\1#' )

# check that input directory is not empty
if [ -z "${INPUT_DIR}" ] ; then
	echo "empty input directory, please do not run $0 directly!"
	exit 1
fi

CMD="${APP} -f ${BASEDIR}/${INPUT_DIR} ${BASEDIR}/${INPUT_DIR}/source.pcap"
echo $CMD
 
# run in verbose mode or quiet mode     
if [ "${VERBOSE}" = "verbose" ] ; then  
	if [ "${VERY_VERBOSE}" = "verbose" ] ; then
		${CMD} -d 1 || exit $?
	else
		${CMD} || exit $?
	fi
else                                    
	${CMD} > /dev/null 2>&1 || exit $?
fi

