#!/bin/sh

set -e

rm -f /tmp/test_sand_collector.log

if [ -f /tmp/sand_collector_pid ]; then
	echo "/tmp/sand_collector_pid already exists; exiting" | tee -a /tmp/test_sand_collector.log
	exit 1
fi

if [ "`echo /tmp/tmp*_opensand_collector`" != '/tmp/tmp*_opensand_collector' ]; then
	echo "Collector log directory/ies already exist; exiting" | tee -a /tmp/test_sand_collector.log
	exit 1
fi

/usr/bin/sand-collector -t _opensand3._tcp >/dev/null 2>&1 &
sleep 5
PID="$!"
echo "Started sand-collector with PID $!" | tee -a /tmp/test_sand_collector.log
echo "$PID" > /tmp/sand_collector_pid

exit 0
