#!/bin/bash

output=$(./reto1_cpu 2 1.0e-6 2>&1)
echo "=== RAW OUTPUT ==="
echo "$output" | tail -10
echo ""
echo "=== EXTRACTIONS ==="
iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')

echo "Iterations: $iterations"
echo "Time: $time_sec"
echo "Error Jacobi: $error_jacobi"
echo "Error Disc: $error_disc"
