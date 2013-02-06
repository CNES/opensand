#!/bin/sh

cd `dirname $0`

gcc -I /usr/include/opensand_output -lopensand_output -pthread -o test_prog test_prog.cpp
