#!/bin/sh

stop_collector() {
	PID=`cat /tmp/sand_collector_pid`
	echo "Stopping collector (PID $PID)" | tee -a /tmp/test_sand_collector.log
	kill -TERM "$PID"
	rm /tmp/sand_collector_pid
	rm -r /tmp/tmp*_opensand_collector
}

check_file_lines() {
	expected_lnum=$1
	name=$2

	if [ ! -f $name ]; then
		echo "$name log file does not exist!"  | tee -a /tmp/test_sand_collector.log
		stop_collector
		exit 1
	fi

	lnum= `wc -l < $name`
	if [ $lnum -ne $expected_lnum ]; then
		echo "$name log file has $lnum lines, expected $expected_lnum." | tee -a /tmp/test_sand_collector.log
		stop_collector
		exit 1
	fi
}

rm /tmp/test_sand_collector.log
find /tmp 2>&1 | tee -a /tmp/test_sand_collector.log

cd /tmp/tmp*_opensand_collector/st1/test_prog/ || (echo "Failed to cd to the test_prog log dir" | tee -a /tmp/test_sand_collector.log ;stop_collector ; exit 1)

echo /tmp/tmp*_opensand_collector/st1/test_prog/ | tee -a /tmp/test_sand_collector.log
pwd | tee -a /tmp/test_sand_collector.log
ls -l | tee -a /tmp/test_sand_collector.log

check_file_lines 0 double_probe.log
check_file_lines 2 event_log.txt
check_file_lines 20 float_probe.log
check_file_lines 0 int32_avg_probe.log
check_file_lines 0 int32_dis_probe.log
check_file_lines 20 int32_last_probe.log
check_file_lines 0 int32_max_probe.log
check_file_lines 0 int32_min_probe.log
check_file_lines 0 int32_sum_probe.log

stop_collector
exit 0
