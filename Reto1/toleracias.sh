#!/bin/bash

set -u

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
cd "$SCRIPT_DIR"

REPEATS="${REPEATS:-10}"
NTHREADS="${NTHREADS:-4}"
NPROC="${NPROC:-4}"
OUTPUT_FILE_TOL="${OUTPUT_FILE_TOL:-benchmark_result_tolerances.csv}"

build_binary_if_needed() {
    local src="$1"
    local out="$2"
    shift 2
    local extra_flags=("$@")

    if [[ ! -x "$out" || "$src" -nt "$out" ]]; then
        echo "Compilando $out desde $src ..."
        gcc "$src" -o "$out" "${extra_flags[@]}"
    fi
}

build_binary_if_needed "Reto1.c" "reto1_jacobi" -lm
build_binary_if_needed "Reto1.c" "reto1_cpu" -Ofast -lm
build_binary_if_needed "Reto1_threads.c" "reto1_threads" -lm -pthread
build_binary_if_needed "Reto1_processes.c" "reto1_processes" -lm -pthread

tolerances=("1.0e-3" "1.0e-4" "1.0e-5" "1.0e-6" "1.0e-7" "1.0e-8")

echo "phase,run,test_type,k,tolerance,nthreads,nproc,iterations,time_seconds,error_jacobi,error_discretization" > "$OUTPUT_FILE_TOL"

for run in $(seq 1 "$REPEATS"); do
    echo ""
    echo "=== Repetición ${run}/${REPEATS} (barrido de tolerancia) ==="

    echo ""
    echo "--- Bloque: secuencial estándar (tol sweep, K=10) ---"
    # Secuencial estándar
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=10, Tolerancia=$tol"
        echo "Ejecutando versión secuencial (estándar)..."
        output=$(./reto1_jacobi 10 "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,secuencial_std,10,$tol,1,1,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: secuencial optimizado -ofast (tol sweep, K=10) ---"
    # Secuencial optimizado
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=10, Tolerancia=$tol"
        echo "Ejecutando versión secuencial (optimizada -ofast)..."
        output=$(./reto1_cpu 10 "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,secuencial_ofast,10,$tol,1,1,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: threads (tol sweep, K=10) ---"
    # Threads
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=10, Tolerancia=$tol"
        echo "Ejecutando versión con threads (n=$NTHREADS)..."
        output=$(./reto1_threads 10 "$NTHREADS" "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,threads,10,$tol,$NTHREADS,0,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: processes (tol sweep, K=10) ---"
    # Procesos
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=10, Tolerancia=$tol"
        echo "Ejecutando versión con procesos (n=$NPROC)..."
        output=$(./reto1_processes 10 "$NPROC" "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,processes,10,$tol,0,$NPROC,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done
done

echo ""
echo "=========================================="
echo "Benchmark completado!"
echo "Resultados barrido tolerancia guardados en: $OUTPUT_FILE_TOL"
echo "=========================================="
