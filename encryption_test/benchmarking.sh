#!/bin/bash

PCI_ADDR="03:00.0"
DOCA_CMD="/doca_build/samples/doca_aes_gcm/aes_gcm_encrypt/doca_aes_gcm_encrypt"
WORKDIR="/tmp/aes_gcm_perf_test"
mkdir -p "$WORKDIR"
cd "$WORKDIR"

SIZES_BYTES=(
64 128 256 512 1024 2048 4096 8192 16384 32768
65536 131072 262144 524288 1048576 2097152 4194304
8388608 16777216 33554432 67108864
134217728 268435456 536870912 1073741824 2147483648
3758096384 
)

echo "Size(B) | Submit Latency (us) | Real Time (ns) | Throughput (Gbps)"
echo "------------------------------------------------------------------"

for SIZE_BYTES in "${SIZES_BYTES[@]}"; do
    PLAINTEXT="plain_${SIZE_BYTES}_B.txt"
    ENCRYPTED="encrypted_${SIZE_BYTES}_B.txt"
    LOGFILE="log_${SIZE_BYTES}_B.txt"

    head -c "$SIZE_BYTES" </dev/urandom > "$PLAINTEXT"

    { time $DOCA_CMD -p $PCI_ADDR -f "$PLAINTEXT" -o "$ENCRYPTED"; } &> "$LOGFILE"

    SUBMIT_NS=$(grep "submit_aes_gcm_encrypt_task latency" "$LOGFILE" | awk '{print $(NF-1)}')
    SUBMIT_US=$(echo "scale=3; $SUBMIT_NS / 1000" | bc)
    EXECUTION_NS=$(grep "aes_gcm_encrypt_task execution time" "$LOGFILE" | awk '{print $(NF-1)}')

    THROUGHPUT_Gbps=$(echo "scale=4; $SIZE_BYTES * 8 / $EXECUTION_NS" | bc)

    printf "%7s | %18s | %13s | %18s\n" "$SIZE_BYTES" "$SUBMIT_US" "$EXECUTION_NS" "$THROUGHPUT_Gbps"

    rm -f "$PLAINTEXT" "$ENCRYPTED" "$LOGFILE"
done
