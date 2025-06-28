#!/bin/bash

DEVICE=mlx5_0
LOG_FILE="server_results.log"
ITERATIONS=1
SIZES=(64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576)
TOOLS=("ib_send_bw" "ib_send_lat" "ib_read_bw" "ib_read_lat" "ib_write_bw" "ib_write_lat")

echo "Server starting..." > $LOG_FILE

for TOOL in "${TOOLS[@]}"; do
    for SIZE in "${SIZES[@]}"; do
        for ((i = 1; i <= ITERATIONS; i++)); do
            echo "Starting server mode for $TOOL with message size $SIZE bytes..." >> $LOG_FILE
            $TOOL -d $DEVICE -s $SIZE -t 1 >> $LOG_FILE 2>&1 &
            echo "$TOOL" >> $LOG_FILE
            SERVER_PID=$!
            
            echo "Server is running with PID $SERVER_PID..." >> $LOG_FILE
            
            # Wait for server to keep running (this can be adjusted as needed)
            wait $SERVER_PID

            echo "" >> $LOG_FILE
        done

        echo "" >> $LOG_FILE
    done

    echo "" >> $LOG_FILE
done

echo "Server test completed." >> $LOG_FILE
