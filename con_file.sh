#!/bin/bash

# This script tests concurrent file transfer
# Run this script from the build directory
# Make sure the server (./file_server) is running before executing this script

# Path to test files (relative to build directory)
FILE_DIR=".."

echo "=== Simple file transfer (5 x 10MB files) ==="
for (( i=1; i<=5; i++ ))
do
    echo "Starting client $i: file$i.zip"
    ./file_client ${FILE_DIR}/file$i.zip &
    sleep 0.1
done
wait
echo "All small file transfers completed."

echo ""
echo "=== Large file transfer (2 x 50MB files) ==="
for (( i=1; i<=2; i++ ))
do
    echo "Starting client $i: bigfile$i.zip"
    ./file_client ${FILE_DIR}/bigfile$i.zip &
    sleep 0.1
done
wait
echo "All large file transfers completed."

echo ""
echo "=== Transfer Summary ==="
echo "Files in current directory:"
ls -lh *.zip 2>/dev/null || echo "No .zip files found"
