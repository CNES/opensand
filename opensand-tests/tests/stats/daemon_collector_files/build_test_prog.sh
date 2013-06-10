#!/bin/sh

cd `dirname $0`

g++ -lopensand_output -lpthread -o test_prog test_prog.cpp
