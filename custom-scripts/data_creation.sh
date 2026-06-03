#!/bin/sh

if [ "$#" -ne 1 ]; then
    echo "Usage: $0 <scheduler_name>"
    echo "Example: $0 scan"
    exit 1
fi

SCHEDULER=$1
DEVICE="/dev/sdb"
TEST_PROGRAM="./disk_scheduler_test"

DATA_DIR="benchmark_results"
mkdir -p "$DATA_DIR"

# Verify if the test program exists
if [ ! -x "$TEST_PROGRAM" ]; then
    echo "Error: Test program '$TEST_PROGRAM' not found or not executable."
    exit 1
fi

echo "Injecting scheduler '$SCHEDULER' into $DEVICE..."
# Configuration: Apply the target scheduler to the block device
echo $SCHEDULER > /sys/block/sdb/queue/scheduler

echo "Starting benchmark battery for: $SCHEDULER"

# ---------------------------------------------------------
# PHASE 1: SEQUENTIAL ACCESS TESTS (1 to 40 workers)
# ---------------------------------------------------------
echo "--- Starting Sequential Tests ---"
for WORKERS in $(seq 1 40); do
    OUTPUT_FILE="${DATA_DIR}/test_${SCHEDULER}_sequential_${WORKERS}.txt"
    echo "Running Sequential Test | Workers: $WORKERS | Output: $OUTPUT_FILE"

    # Configuration: Clear kernel ring buffer
    dmesg -c > /dev/null
    
    # Configuration: Drop page cache to force real physical disk reads
    echo 3 > /proc/sys/vm/drop_caches

    # Run test and pipe user-space logs directly into kernel logs
    $TEST_PROGRAM $DEVICE 1 $WORKERS | tee /dev/kmsg > /dev/null

    # Export synchronized logs to the final text file
    dmesg > $OUTPUT_FILE

    ./data_extractor "$OUTPUT_FILE" "$SCHEDULER" "Sequential" "$WORKERS"
done

# ---------------------------------------------------------
# PHASE 2: RANDOM ACCESS TESTS (1 to 40 workers)
# ---------------------------------------------------------
echo "--- Starting Random Tests ---"
for WORKERS in $(seq 1 40); do
    OUTPUT_FILE="${DATA_DIR}/test_${SCHEDULER}_random_${WORKERS}.txt"
    echo "Running Random Test | Workers: $WORKERS | Output: $OUTPUT_FILE"

    # Configuration: Clear kernel ring buffer
    dmesg -c > /dev/null
    
    # Configuration: Drop page cache to force real physical disk reads
    echo 3 > /proc/sys/vm/drop_caches

    # Run test and pipe user-space logs directly into kernel logs
    $TEST_PROGRAM $DEVICE 2 $WORKERS | tee /dev/kmsg > /dev/null

    # Export synchronized logs to the final text file
    dmesg > $OUTPUT_FILE

    ./data_extractor "$OUTPUT_FILE" "$SCHEDULER" "Sequential" "$WORKERS"
done

echo "Benchmark battery completed successfully! Files generated."
