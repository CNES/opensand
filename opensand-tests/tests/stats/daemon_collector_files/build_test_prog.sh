#!/bin/sh

cd `dirname $0`

gcc -I /usr/include/opensand_env_plane -lopensand_env_plane -pthread -o test_prog test_prog.cpp
