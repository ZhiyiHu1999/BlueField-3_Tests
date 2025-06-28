#!/bin/bash

DEVICE=mlx5_0
HOST=10.253.74.54
LOG_FILE="client_results.log"
ITERATIONS=1
SIZES=(64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576)
TOOLS=("ib_send_bw" "ib_send_lat" "ib_read_bw" "ib_read_lat" "ib_write_bw" "ib_write_lat")

echo "Cleaning old log file..." > $LOG_FILE

echo "Starting RDMA performance test: $(date)" >> $LOG_FILE
echo "Device: $DEVICE" >> $LOG_FILE
echo "" >> $LOG_FILE

for TOOL in "${TOOLS[@]}"; do
    echo "Running $TOOL test..." >> $LOG_FILE
    
    for SIZE in "${SIZES[@]}"; do
        echo "Testing message size: $SIZE bytes" >> $LOG_FILE
        
        for ((i = 1; i <= ITERATIONS; i++)); do
            echo "Running iteration $i..." >> $LOG_FILE
            echo "Test type: $TOOL" >> $LOG_FILE

            sleep 2

            $TOOL -d $DEVICE -s $SIZE -t 1 $HOST >> $LOG_FILE 2>&1
            echo "$TOOL" >> $LOG_FILE
            CLIENT_PID=$!
            wait $CLIENT_PID

            echo "" >> $LOG_FILE
        done

        echo "" >> $LOG_FILE
    done

    echo "" >> $LOG_FILE
done

echo "All tests completed, results saved to $LOG_FILE"
