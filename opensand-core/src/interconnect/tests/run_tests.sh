#!/bin/bash

# There has to be an open permanent ssh session with hosts,
# otherwise, there'll be a prompt for the password at each iteration.

HOST_A_IP="172.20.42.10"
HOST_A_USER="satsix"
HOST_B_IP="172.20.42.11"
HOST_B_USER="satsix"

N_ITER=30

# First, tests with no interconnect block
echo "No interconnect block."

for i in $(seq 1 $N_ITER)
do
    ssh $HOST_A_USER@$HOST_A_IP 'cd test_opensand && ./test_no_interconnect -i in.dat -o out.dat 1>/dev/null' 2>&1
done

# Then, tests with interconnect block in same machine
echo "With interconnect block (same machine)"

for i in $(seq 1 $N_ITER)
do
    ssh $HOST_A_USER@$HOST_A_IP 'cd test_opensand && ./test_interconnect_top -i in.dat -o out.dat -u 10001 -d 10002 1>/dev/null 2>results.log' &
    sleep 1
    ssh $HOST_A_USER@$HOST_A_IP 'cd test_opensand && ./test_interconnect_bottom -i 127.0.0.1 -u 10001 -d 10002' 1>/dev/null 2>&1
    RESULT=""
    while [[ -z $RESULT ]]; do sleep 1 ; RESULT=$(ssh $HOST_A_USER@$HOST_A_IP 'cat test_opensand/results.log'); done
    echo $RESULT
    sleep 6
done

# Finally, tests with interconnect block in different machine
echo "With interconnect block (diff machine)"

for i in $(seq 1 $N_ITER)
do
    ssh $HOST_A_USER@$HOST_A_IP 'cd test_opensand && ./test_interconnect_top -i in.dat -o out.dat -u 10001 -d 10002 1>/dev/null 2>results.log' &
    sleep 1
    ssh $HOST_B_USER@$HOST_B_IP "cd test_opensand && ./test_interconnect_bottom -i ${HOST_A_IP} -u 10001 -d 10002" 1>/dev/null 2>&1
    RESULT=""
    while [[ -z $RESULT ]]; do sleep 1 ; RESULT=$(ssh $HOST_A_USER@$HOST_A_IP 'cat test_opensand/results.log'); done
    echo $RESULT
    sleep 6
done
