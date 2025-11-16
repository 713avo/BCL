#!/bin/bash
# Script to run all tests and collect results

echo "=========================================="
echo " BCL Test Suite - Complete Run"
echo "=========================================="
echo ""

TOTAL=0
PASSED=0
FAILED=0

for test in tests/test_*.bcl; do
    echo "Running: $test"
    TOTAL=$((TOTAL + 1))

    if ./bin/bcl "$test" 2>&1; then
        PASSED=$((PASSED + 1))
    else
        FAILED=$((FAILED + 1))
        echo "  ^^^ Test had errors ^^^"
    fi
    echo ""
done

echo "=========================================="
echo " Test Summary"
echo "=========================================="
echo "Total Suites: $TOTAL"
echo "Passed: $PASSED"
echo "Failed: $FAILED"
echo "=========================================="
