#!/bin/bash

if [ $# != 2 ] ; then
    echo Usage: $0 writefile writestr
    exit 1
fi

make clean
make 
./writer "$1" "$2"
