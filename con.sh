#!/bin/bash

# This script tests concurrent client connections
# Run this script from the build directory
# Make sure the server (./socket_server) is running before executing this script

CLIENT_COUNT=30

echo "Starting $CLIENT_COUNT concurrent clients..."
for (( i=1; i<=CLIENT_COUNT; i++ ))
do
    echo "Starting client $i"
    ./socket_client &
    sleep 0.1
done

wait
echo "All $CLIENT_COUNT clients have finished."
