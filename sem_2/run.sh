#!/bin/bash

time dd if=/dev/urandom of="4gb_test.txt" bs=1048576 count=4096

IN="4gb_test.txt"
OUT="dupp_output.txt"

if [ ! -f "$IN" ]; then
    echo "Error: input file '$IN' not found!"
    exit 1
fi

if [ ! -f "$OUT" ]; then
    echo "Error: output '$OUT' not found!"
    exit 1
fi

echo "Start compiling ---------------------------------------------------------"
gcc -Iinclude task_2_pipe.cpp task_2_main.cpp -o test
echo "End compiling   ---------------------------------------------------------"
./test

HASH_IN=$(md5sum "$IN" | awk '{print $1}')
HASH_OUT=$(md5sum "$OUT" | awk '{print $1}')

if [ -z "$HASH_IN" ]; then
    echo "Error: can't receive hash of input file '$IN'"
    exit 1
fi

if [ -z "$HASH_OUT" ]; then
    echo "Error: can't receive hash of output file '$OUT'"
    exit 1
fi

echo "Input hash:  $HASH_IN"
echo "Output hash: $HASH_OUT"

if [ "$HASH_IN" = "$HASH_OUT" ]; then
    echo "Files are identical!"
else
    echo "Files differ!"
fi


