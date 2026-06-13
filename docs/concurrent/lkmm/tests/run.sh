#!/bin/bash
# LKMM Litmus Tests Runner
# Usage: ./run.sh [test.litmus]

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TESTS_DIR="$SCRIPT_DIR/tests"
RESULTS_DIR="$SCRIPT_DIR/results"

# Find kernel memory-model path
KERNEL_LKMM=""
if [ -d "$HOME/data/kernel/linux/tools/memory-model" ]; then
    KERNEL_LKMM="$HOME/data/kernel/linux/tools/memory-model"
elif [ -d "/lib/modules/$(uname -r)/build/tools/memory-model" ]; then
    KERNEL_LKMM="/lib/modules/$(uname -r)/build/tools/memory-model"
else
    echo "ERROR: Cannot find kernel tools/memory-model directory"
    echo "Please set KERNEL_LKMM environment variable"
    exit 1
fi

# Find herd stdlib path
HERD_STDLIB=""
if command -v herd7 &> /dev/null; then
    HERD_SHARE="$(dirname "$(command -v herd7)")/../share/herdtools7/herd"
    HERD_STDLIB="$(cd "$HERD_SHARE" && pwd)"
fi

if [ -z "$HERD_STDLIB" ] || [ ! -f "$HERD_STDLIB/stdlib.cat" ]; then
    echo "ERROR: Cannot find herd7 stdlib.cat"
    echo "Please install herdtools7: opam install herdtools7"
    exit 1
fi

mkdir -p "$RESULTS_DIR"

run_test() {
    local test_file="$1"
    local test_name="$(basename "$test_file" .litmus)"
    local result_file="$RESULTS_DIR/$test_name.txt"

    echo "========================================"
    echo "Running: $test_name"
    echo "========================================"

    herd7 -conf "$KERNEL_LKMM/linux-kernel.cfg" \
          -I "$HERD_STDLIB" \
          -I "$KERNEL_LKMM" \
          "$test_file" 2>&1 | tee "$result_file"

    # Extract result
    local observation=$(grep "^Observation" "$result_file" | awk '{print $2, $3}')
    echo ""
    echo "Result: $observation"
    echo "Saved to: $result_file"
    echo ""
}

# Main
if [ $# -eq 1 ]; then
    # Run single test
    if [ -f "$1" ]; then
        run_test "$1"
    elif [ -f "$TESTS_DIR/$1" ]; then
        run_test "$TESTS_DIR/$1"
    else
        echo "ERROR: Test file not found: $1"
        exit 1
    fi
else
    # Run all tests
    echo "LKMM Path: $KERNEL_LKMM"
    echo "Herd Stdlib: $HERD_STDLIB"
    echo ""

    for test in "$TESTS_DIR"/*.litmus; do
        if [ -f "$test" ]; then
            run_test "$test"
        fi
    done

    echo "========================================"
    echo "All tests completed. Results in: $RESULTS_DIR"
    echo "========================================"
fi
