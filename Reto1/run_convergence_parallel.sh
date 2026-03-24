#!/bin/bash

# Script para estudiar convergencia del método Jacobi 1D
# en versiones de hilos y procesos.
# Mantiene la misma lógica de run_convergence_study.sh

cd "$(dirname "$0")" || exit 1

# Parámetros configurables (puedes sobreescribir por variable de entorno)
RUNS=${RUNS:-10}
WORKERS=${WORKERS:-4}
K_VALUES=${K_VALUES:-"1 2 3 4 5 7 8 9 10 11 12 13 14 15 16 17"}
OUTPUT_FILE=${OUTPUT_FILE:-"convergence_parallel_results.csv"}

# Compilar sin optimizaciones
echo "Compilando Reto1_threads.c y Reto1_processes.c sin optimizaciones..."
gcc Reto1_threads.c -O0 -lm -pthread -o reto1_threads
if [ $? -ne 0 ]; then
    echo "Error: no se pudo compilar Reto1_threads.c"
    exit 1
fi

gcc Reto1_processes.c -O0 -lm -o reto1_processes
if [ $? -ne 0 ]; then
    echo "Error: no se pudo compilar Reto1_processes.c"
    exit 1
fi

echo ""
echo "========================================================"
echo "Convergencia Jacobi 1D (Hilos y Procesos)"
echo "========================================================"
echo "Workers por ejecución: $WORKERS"
echo "Guardando resultados en: $OUTPUT_FILE"
echo ""

# Crear CSV con encabezado si no existe
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Run,Mode,Workers,K,NK,Iteraciones,Tiempo(s),Error_Jacobi,Error_Discretizacion" > "$OUTPUT_FILE"
fi

for run in $(seq 1 "$RUNS"); do
    echo ""
    echo "========== RUN $run / $RUNS =========="
    echo "Modo      K    NK        Iteraciones    Tiempo(s)       Error_Jacobi    Error_Discretización"
    echo "--------  ---  ---------- -----------    -----------     -----------     --------------------"

    for mode in threads processes; do
        if [ "$mode" = "threads" ]; then
            exe="./reto1_threads"
            label="THREADS"
        else
            exe="./reto1_processes"
            label="PROCESSES"
        fi

        for k in $K_VALUES; do
            # Ejecutar y extraer métricas
            output=$($exe "$k" "$WORKERS" 2>/dev/null)

            nk=$(echo "$output" | grep "System size NK was" | awk '{print $NF}')
            it=$(echo "$output" | grep "Number of Jacobi iterations required was" | awk '{print $NF}')
            tiempo=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
            e_jacobi=$(echo "$output" | grep "RMS Jacobi error in solution" | awk '{print $NF}')
            e_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')

            # Imprimir en terminal
            printf "%-8s  %-3d  %-10s %-14s %-15s %-15s %s\n" "$label" "$k" "$nk" "$it" "$tiempo" "$e_jacobi" "$e_disc"

            # Guardar en CSV
            echo "$run,$mode,$WORKERS,$k,$nk,$it,$tiempo,$e_jacobi,$e_disc" >> "$OUTPUT_FILE"
        done
    done
done

echo ""
echo "Estudio completado."
echo "Resultados guardados en: $OUTPUT_FILE"

sudo shutdown now