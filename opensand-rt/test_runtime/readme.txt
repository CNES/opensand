
This directory contains some sources to test the runtime behavior.

call ./test1 to build the app and test NetSocket and Timer events, with 1 block 
call ./test2 to build the app and test Message and signal events, with 3 blocks


test1 reads a file and writes to another in both direction, then exits after 10 timeouts if 100ms. 
After that is compares in and out files (except timer log)

input file :file_in_forward.txt, file_in_backward.txt
output files: file_out_forward.txt, file_out_backward.txt, timer_out_forward.txt, timer_out_backward.txt

test2 reads a file and writes to another in both direction, 
and exits upon receiving the SIGUSR1 signal
After that it compares in and out files

input file :file_in_forward.txt, file_in_backward.txt
output files: file_out_3blocks_forward.txt, file_out_3blocks_backward.txt
   

