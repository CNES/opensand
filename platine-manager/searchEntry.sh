#!/bin/bash

string=$1
value=$2
conf=$3

for line in `cat $conf | grep "$string"` ; do
    if [ ${line:0:1} != "#" ] ; then
        if [ "$value" != "" ] ; then
            echo -e "\t\t\tset parameter '$string' to value '$value'."
            sed -i -e "s/^[ \t]*$string[ \t]*=.*$/$string=$value/" $conf 
        fi
    fi
done
