#!/bin/sh
#
# file:        test_configuration.sh
# description: Check the configuration parsing library.
# author:      Audric Schiltknecht <audric.schiltknecht@toulouse.viveris.com>
#
# Script arguments:
#    test_configuration.sh [verbose]
# where:
#   verbose          prints the traces of test application
#

# parse arguments
SCRIPT="$0"
if [ "x$MAKELEVEL" != "x" ] ; then
	BASEDIR="${srcdir}"
	APP="./test_configuration"
else
	BASEDIR=$( dirname "${SCRIPT}" )
	APP="${BASEDIR}/test_configuration"
fi

CMD="${APP} -i ${BASEDIR}/input/test.xml -i ${BASEDIR}/input/test2.xml -r ${BASEDIR}/input/result"

# run in verbose mode or quiet mode
if [ "${VERBOSE}" = "verbose" ] ; then
		${CMD} || exit $?
else
		${CMD} > /dev/null 2>&1 || exit $?
fi

