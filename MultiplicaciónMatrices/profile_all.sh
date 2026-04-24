#!/bin/bash
# Script para perfilado de matrix_multiply_openmp
# Uso: ./profile_all.sh <tamano_matriz>

BIN=./matrix_multiply_openmp
SIZE=${1:-1000}

# 1. Perfilado de CPU con gprof
echo "Compilando con -pg para gprof..."
gcc -O2 -fopenmp -pg matrix_multiply_openmp.c -o matrix_multiply_openmp_gprof

echo "Ejecutando con gprof..."
./matrix_multiply_openmp_gprof $SIZE
if [ -f gmon.out ]; then
    gprof ./matrix_multiply_openmp_gprof gmon.out > gprof_report.txt
    echo "Reporte CPU (gprof): gprof_report.txt"
fi

# 2. Perfilado de memoria con valgrind memcheck
echo "Perfilando memoria con valgrind..."
valgrind --leak-check=full --log-file=valgrind_memcheck.txt $BIN $SIZE

echo "Reporte memoria (valgrind): valgrind_memcheck.txt"

# 3. Perfilado de uso de memoria pico con massif
echo "Perfilando uso de memoria pico con massif..."
valgrind --tool=massif --massif-out-file=massif.out $BIN $SIZE
ms_print massif.out > massif_report.txt

echo "Reporte uso de memoria pico: massif_report.txt"

# 4. Perfilado de caché con cachegrind
echo "Perfilando caché con cachegrind..."
valgrind --tool=cachegrind --cachegrind-out-file=cachegrind.out $BIN $SIZE
cg_annotate cachegrind.out > cachegrind_report.txt

echo "Reporte caché: cachegrind_report.txt"

# 5. Perfilado de concurrencia con helgrind
echo "Perfilando concurrencia con helgrind..."
valgrind --tool=helgrind --log-file=helgrind_report.txt $BIN $SIZE

echo "Reporte concurrencia: helgrind_report.txt"

# 6. Perfilado de bajo nivel con perf (CPU, cache, RAM)
echo "Perfilando bajo nivel con perf..."
perf stat -e cycles,instructions,cache-references,cache-misses,context-switches,minor-faults,major-faults $BIN $SIZE 2> perf_report.txt

echo "Reporte perf: perf_report.txt"

echo "\nTodos los reportes generados. Revisa los archivos *_report.txt."
