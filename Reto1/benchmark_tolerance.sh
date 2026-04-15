#!/bin/bash

# Script para ejecutar benchmarks variando K y tolerancia
# Parte 1: K del 1 al 15 con tolerancia estándar
# Parte 2: K=12 con tolerancias diferentes

OUTPUT_FILE="benchmark_results_v2.csv"
OUTPUT_FILE_TOL="benchmark_results_tolerance.csv"
NTHREADS=4
NPROC=4
REPEATS=10

# Crear archivo de salida con encabezado
echo "phase,run,test_type,k,tolerance,nthreads,nproc,iterations,time_seconds,error_jacobi,error_discretization" > "$OUTPUT_FILE"
echo "phase,run,test_type,k,tolerance,nthreads,nproc,iterations,time_seconds,error_jacobi,error_discretization" > "$OUTPUT_FILE_TOL"

echo "=========================================="
echo "PARTE 1: Variando K (1-15) con tolerancia estándar"
echo "=========================================="

for run in $(seq 1 "$REPEATS"); do
    echo ""
    echo "=== Repetición ${run}/${REPEATS} (barrido de K) ==="

    echo ""
    echo "--- Bloque: secuencial estándar (K=10..11) ---"
    # Secuencial (reto1_jacobi)
    for k in $(seq 10 11); do
        echo ""
        echo "K=$k, Tolerancia=1.0e-6"
        echo "Ejecutando versión secuencial (estándar)..."
        output=$(./reto1_jacobi "$k" 1.0e-6 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "k_sweep,$run,secuencial_std,$k,1.0e-6,1,1,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: secuencial optimizado -ofast (K=10..11) ---"
    # Secuencial optimizado (reto1_cpu con -ofast)
    for k in $(seq 10 11); do
        echo ""
        echo "K=$k, Tolerancia=1.0e-6"
        echo "Ejecutando versión secuencial (optimizada -ofast)..."
        output=$(./reto1_cpu "$k" 1.0e-6 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "k_sweep,$run,secuencial_ofast,$k,1.0e-6,1,1,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: threads (K=10..11) ---"
    # Threads
    for k in $(seq 10 11); do
        echo ""
        echo "K=$k, Tolerancia=1.0e-6"
        echo "Ejecutando versión con threads (n=$NTHREADS)..."
        output=$(./reto1_threads "$k" "$NTHREADS" 1.0e-6 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "k_sweep,$run,threads,$k,1.0e-6,$NTHREADS,0,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: processes (K=10..11) ---"
    # Procesos
    for k in $(seq 10 11); do
        echo ""
        echo "K=$k, Tolerancia=1.0e-6"
        echo "Ejecutando versión con procesos (n=$NPROC)..."
        output=$(./reto1_processes "$k" "$NPROC" 1.0e-6 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "k_sweep,$run,processes,$k,1.0e-6,0,$NPROC,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done
done

echo ""
echo "=========================================="
echo "PARTE 2: K=12 con tolerancias variables"
echo "=========================================="

tolerances=("1.0e-3" "1.0e-4" "1.0e-5" "1.0e-6" "1.0e-7" "1.0e-8")

for run in $(seq 1 "$REPEATS"); do
    echo ""
    echo "=== Repetición ${run}/${REPEATS} (barrido de tolerancia) ==="

    echo ""
    echo "--- Bloque: secuencial estándar (tol sweep, K=11) ---"
    # Secuencial estándar
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=11, Tolerancia=$tol"
        echo "Ejecutando versión secuencial (estándar)..."
        output=$(./reto1_jacobi 11 "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,secuencial_std,11,$tol,1,1,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: secuencial optimizado -ofast (tol sweep, K=11) ---"
    # Secuencial optimizado
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=11, Tolerancia=$tol"
        echo "Ejecutando versión secuencial (optimizada -ofast)..."
        output=$(./reto1_cpu 11 "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,secuencial_ofast,11,$tol,1,1,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: threads (tol sweep, K=11) ---"
    # Threads
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=11, Tolerancia=$tol"
        echo "Ejecutando versión con threads (n=$NTHREADS)..."
        output=$(./reto1_threads 11 "$NTHREADS" "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,threads,11,$tol,$NTHREADS,0,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done

    echo ""
    echo "--- Bloque: processes (tol sweep, K=11) ---"
    # Procesos
    for tol in "${tolerances[@]}"; do
        echo ""
        echo "K=11, Tolerancia=$tol"
        echo "Ejecutando versión con procesos (n=$NPROC)..."
        output=$(./reto1_processes 11 "$NPROC" "$tol" 2>&1)
        iterations=$(echo "$output" | grep "Number of Jacobi iterations" | awk '{print $NF}')
        time_sec=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        error_jacobi=$(echo "$output" | grep "RMS Jacobi error" | awk '{print $NF}')
        error_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        echo "tol_sweep,$run,processes,11,$tol,0,$NPROC,$iterations,$time_sec,$error_jacobi,$error_disc" >> "$OUTPUT_FILE_TOL"
        echo "  Iteraciones: $iterations, Tiempo: ${time_sec}s"
    done
done

echo ""
echo "=========================================="
echo "Benchmark completado!"
echo "Resultados barrido K guardados en: $OUTPUT_FILE"
echo "Resultados barrido tolerancia guardados en: $OUTPUT_FILE_TOL"
echo "=========================================="
