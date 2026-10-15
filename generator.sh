#!/bin/bash

# This script generates test files for the file transfer assignment
# Run this script from the Assignment-2 directory

echo "Generating small files (5 x 10MB)..."
for (( i=1; i<=5; i++ ))
do
    dd if=/dev/urandom of=file$i.zip bs=1000000 count=10 2>/dev/null &
done
wait
echo "Finished generating small files."

echo "Generating large files (2 x 50MB)..."
for (( i=1; i<=2; i++ ))
do
    dd if=/dev/urandom of=bigfile$i.zip bs=1000000 count=50 2>/dev/null &
done
wait
echo "Finished generating large files."

echo "All test files generated successfully!"
ls -lh *.zip 2>/dev/null