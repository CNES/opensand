#!/bin/sh
#
# file:        test.sh
# description: Check the opensand-rt library
# author:      Julien BERNARD <jbernard@toulouse.viveris.com>
#
# Script:
#    test.sh [verbose]
#

# parse arguments
SCRIPT="$0"
if [ "x$MAKELEVEL" != "x" ] ; then
	BASEDIR="${srcdir}"
	TEST="./test_block"
	TEST_MULTI="./test_multi_blocks -i ${BASEDIR}/TestMultiBlocks.h"
	TEST_MUX="./test_mux_blocks"
else
	BASEDIR=$( dirname "${SCRIPT}" )
	TEST="${BASEDIR}/test_block"
	TEST_MULTI="${BASEDIR}/test_multi_blocks -i ${BASEDIR}/TestMultiBlocks.h"
	TEST_MUX="${BASEDIR}/test_mux_blocks"
fi

if [ -e "/usr/bin/google-pprof" ]; then
	# we need that because debian rename pprof in packages...
	export PPROF_PATH="/usr/bin/google-pprof"
fi

# check for heaps and restart with logs if an error occurs
echo "Check block"
env HEAPCHECK=strict > /dev/null ${TEST} 2>&1 1>/dev/null || env HEAPCHECK=strict ${TEST} || exit $?
if [ "$?" -ne "0" ]; then
    exit 1
fi
echo "Check multi blocks"
env HEAPCHECK=strict > /dev/null ${TEST_MULTI} 2>&1 1>/dev/null || env HEAPCHECK=strict ${TEST_MULTI} || exit $?

echo "Check mux blocks"
env HEAPCHECK=strict > /dev/null "${TEST_MUX}" 2>&1 1>/dev/null || env HEAPCHECK=strict "${TEST_MUX}" || exit $?
