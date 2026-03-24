#!/bin/bash
cd "$(dirname "$0")" || exit 1
gcc Reto1.c -O0 -lm -o reto1_jacobi

echo ""
echo "K     NK        Iteraciones    Tiempo (s)      Error_Jacobi    Error_Disco"
echo "---   ---------- -----------    ----------      -----------     ----------"

for k in 5 10 15; do
    output=$(./reto1_jacobi "$k" 2>/dev/null)
    nk=$(echo "$output" | grep "System size NK was" | awk '{print $NF}')
    it=$(echo "$output" | grep "Number of Jacobi iterations required was" | awk '{print $NF}')
    time_val=$(echo "$output" | grep "Elapsed time" | awk '{print $NF}')
    e_jacobi=$(echo "$output" | grep "RMS Jacobi error in solution" | awk '{print $NF}')
    e_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
    printf "%-5d %-10d %-14d %-15s %-15s %s\n" "$k" "$nk" "$it" "$time_val" "$e_jacobi" "$e_disc"
done
