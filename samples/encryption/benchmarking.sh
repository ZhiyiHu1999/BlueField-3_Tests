#!/bin/bash

PCI_ADDR="03:00.0"
DOCA_CMD="/tmp/build_samples/aes_gcm_encrypt/doca_aes_gcm_encrypt"
WORKDIR="/tmp/aes_gcm_perf_test"
mkdir -p "$WORKDIR"
cd "$WORKDIR"

SIZES_BYTES=(64 128 256 512 1024 2048 4096 8192 16384 32768 65536 131072 262144 524288 1048576)

echo "Size(B) | Submit Latency (ms) | Real Time (s) | Throughput (B/s)"
echo "------------------------------------------------------------------"

for SIZE_BYTES in "${SIZES_BYTES[@]}"; do
    PLAINTEXT="plain_${SIZE_BYTES}_B.txt"
    ENCRYPTED="encrypted_${SIZE_BYTES}_B.txt"
    LOGFILE="log_${SIZE_BYTES}_B.txt"

    head -c "$SIZE_BYTES" </dev/urandom > "$PLAINTEXT"

    { time $DOCA_CMD -p $PCI_ADDR -f "$PLAINTEXT" -o "$ENCRYPTED"; } &> "$LOGFILE"

    SUBMIT_NS=$(grep "submit_aes_gcm_encrypt_task latency" "$LOGFILE" | awk '{print $(NF-1)}')
    SUBMIT_MS=$(echo "scale=3; $SUBMIT_NS / 1000000" | bc)

    REAL_SEC=$(grep real "$LOGFILE" | awk '{print $2}')
    MIN=$(echo "$REAL_SEC" | cut -dm -f1)
    SEC=$(echo "$REAL_SEC" | cut -dm -f2 | sed 's/s//')
    TOTAL_SEC=$(echo "$MIN * 60 + $SEC" | bc)

    THROUGHPUT_BPS=$(echo "scale=2; $SIZE_BYTES / $TOTAL_SEC" | bc)

    printf "%7s | %18s | %13s | %18s\n" "$SIZE_BYTES" "$SUBMIT_MS" "$TOTAL_SEC" "$THROUGHPUT_BPS"

    rm -f "$PLAINTEXT" "$ENCRYPTED" "$LOGFILE"
done
