#!/bin/sh

cd `dirname $0`

g++ `pkg-config opensand_output --cflags` test_prog.cpp `pkg-config opensand_output --libs` -lpthread -o test_prog
