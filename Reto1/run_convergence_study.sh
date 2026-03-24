#!/bin/bash

# Script para estudiar convergencia del método Jacobi para Poisson 1D
# Corre desde k=5 hasta k=50 con saltos de 5

cd "$(dirname "$0")" || exit 1

# Compilar sin optimizaciones
echo "Compilando Reto1.c sin optimizaciones..."
gcc Reto1.c -O0 -lm -o reto1_jacobi
if [ $? -ne 0 ]; then
    echo "Error: no se pudo compilar."
    exit 1
fi

# Arquivo de salida
OUTPUT_FILE="convergence_results.csv"

echo ""
echo "=========================================="
echo "Estudio de Convergencia: Jacobi Poisson 1D"
echo "=========================================="
echo ""
echo "Guardando resultados en: $OUTPUT_FILE"
echo ""

# Crear archivo CSV con encabezado si no existe
if [ ! -f "$OUTPUT_FILE" ]; then
    echo "Run,K,NK,Iteraciones,Tiempo(s),Error_Jacobi,Error_Discretizacion" > "$OUTPUT_FILE"
fi

# Loop externo: repetir 10 veces
for run in {1..10}; do
    echo ""
    echo "========== RUN $run / 10 =========="
    echo "K    NK        Iteraciones    Tiempo(s)       Error_Jacobi    Error_Discretización"
    echo "---  ---------- -----------    -----------     -----------     --------------------"
    
    for k in 1 2 3 4 5 5 7 8 9 10 11 12 13 14 15 16 17; do
        # Ejecutar y extraer métricas
        output=$(./reto1_jacobi "$k" 2>/dev/null)
        
        # Extraer NK
        nk=$(echo "$output" | grep "System size NK was" | awk '{print $NF}')
        
        # Extraer número de iteraciones
        it=$(echo "$output" | grep "Number of Jacobi iterations required was" | awk '{print $NF}')
        
        # Extraer tiempo
        tiempo=$(echo "$output" | grep "Walltime (MONOTONIC)" | awk '{print $(NF-1)}')
        
        # Extraer error Jacobi
        e_jacobi=$(echo "$output" | grep "RMS Jacobi error in solution" | awk '{print $NF}')
        
        # Extraer error de discretización
        e_disc=$(echo "$output" | grep "RMS discretization error" | awk '{print $NF}')
        
        # Imprimir en terminal
        printf "%-5d %-10d %-14d %-15s %-15s %s\n" "$k" "$nk" "$it" "$tiempo" "$e_jacobi" "$e_disc"
        
        # Guardar en CSV con número de run
        echo "$run,$k,$nk,$it,$tiempo,$e_jacobi,$e_disc" >> "$OUTPUT_FILE"
    done
done

echo ""
echo "Estudio completado."
echo "Resultados guardados en: $OUTPUT_FILE"
