#!/bin/bash

echo "Checking if input file exists..."
if [ ! -f input_file.txt ]; then
    echo "Generating test file..."
    dd if=/dev/urandom of=input_file.txt bs=1048576 count=4096 status=none
fi

echo "Running duplex_pipe..."
./build/duplex_pipe < input_file.txt > output_file.txt

input_md5=$(md5sum input_file.txt | cut -d' ' -f1)
output_md5=$(md5sum output_file.txt | cut -d' ' -f1)

if [ "$input_md5" = "$output_md5" ]; then
    echo "OK"
else
    echo "FAIL"
    echo "input_file.txt: $input_md5"
    echo "output_file.txt:  $output_md5"
    exit 1
fi
