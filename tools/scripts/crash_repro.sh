#!/bin/bash
set -e
SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"
for var in `ls $1|grep $2`
do 
    echo "$1$var"
    sudo cp "$1$var" afl_input
    sudo $SCRIPT_DIR/../fuzz_auto -d tests/necofuzz/
done