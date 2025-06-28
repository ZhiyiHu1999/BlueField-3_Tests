#!/bin/bash

DEVICE=mlx5_0
HOST=127.0.0.1
LOG_FILE="perftest_results.log"
ITERATIONS=5
SIZES=(64 256 1024 4096)
TOOLS=("ib_write_bw" "ib_read_bw" "ib_send_bw")

echo "Cleaning old log file..."
> $LOG_FILE

echo "Starting RDMA performance test: $(date)" >> $LOG_FILE
echo "Device: $DEVICE" >> $LOG_FILE
echo "------------------------------------------" >> $LOG_FILE

for TOOL in "${TOOLS[@]}"; do
    echo "Running $TOOL test..." >> $LOG_FILE
    
    for SIZE in "${SIZES[@]}"; do
        echo "Testing message size: $SIZE bytes" >> $LOG_FILE
        
        for ((i = 1; i <= ITERATIONS; i++)); do
            echo "Starting server mode..." >> $LOG_FILE
            $TOOL -d $DEVICE -s $SIZE -t 1 > server_log.txt 2>&1 &
            
            sleep 2  # Wait for server to start

            echo "Running iteration $i..." >> $LOG_FILE
            if [[ "$TOOL" == "ib_write_bw" ]]; then
                echo "Test type: ib_write_bw" >> $LOG_FILE
            elif [[ "$TOOL" == "ib_read_bw" ]]; then
                echo "Test type: ib_read_bw" >> $LOG_FILE
            elif [[ "$TOOL" == "ib_send_bw" ]]; then
                echo "Test type: ib_send_bw" >> $LOG_FILE
            fi

            $TOOL -d $DEVICE -s $SIZE -t 1 $HOST >> $LOG_FILE 2>&1
            CLIENT_PID=$!
            wait $CLIENT_PID

            echo "------------------------------------------" >> $LOG_FILE
        done

        echo "------------------------------------------" >> $LOG_FILE
    done
done

echo "Test finished" >> $LOG_FILE
echo "------------------------------------------" >> $LOG_FILE

echo "All tests completed, results saved to $LOG_FILE"
