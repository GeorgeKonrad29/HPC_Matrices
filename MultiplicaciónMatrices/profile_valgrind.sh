#!/usr/bin/env bash

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$SCRIPT_DIR/matrix_multiply_openmp.c"
BIN="$SCRIPT_DIR/matrix_multiply_openmp_profile"
GPROF_BIN="$SCRIPT_DIR/matrix_multiply_openmp_gprof_profile"
RESULTS_DIR="$SCRIPT_DIR/profiling_results_$(date +%Y%m%d_%H%M%S)"

SIZE="${1:-300}"
THREADS="${2:-${OMP_NUM_THREADS:-$(nproc)}}"

if ! command -v gcc >/dev/null 2>&1; then
    echo "Error: gcc no está instalado o no está en PATH." >&2
    exit 1
fi

if ! command -v valgrind >/dev/null 2>&1; then
    echo "Error: valgrind no está instalado o no está en PATH." >&2
    exit 1
fi

if ! command -v gprof >/dev/null 2>&1; then
    echo "Error: gprof no está instalado o no está en PATH." >&2
    exit 1
fi

mkdir -p "$RESULTS_DIR"

echo "== Compilando =="
gcc -O2 -g -fopenmp "$SRC" -o "$BIN"
gcc -O2 -g -pg -fopenmp "$SRC" -o "$GPROF_BIN"

export OMP_NUM_THREADS="$THREADS"

echo "== Ejecutando profiling de CPU =="
valgrind --tool=callgrind \
    --callgrind-out-file="$RESULTS_DIR/callgrind.out" \
    "$BIN" "$SIZE" > "$RESULTS_DIR/callgrind_stdout.txt" 2>&1

if command -v callgrind_annotate >/dev/null 2>&1; then
    callgrind_annotate "$RESULTS_DIR/callgrind.out" > "$RESULTS_DIR/callgrind_annotated.txt"
fi

echo "== Ejecutando profiling de caché =="
valgrind --tool=cachegrind \
    --cachegrind-out-file="$RESULTS_DIR/cachegrind.out" \
    "$BIN" "$SIZE" > "$RESULTS_DIR/cachegrind_stdout.txt" 2>&1

if command -v cg_annotate >/dev/null 2>&1; then
    cg_annotate "$RESULTS_DIR/cachegrind.out" > "$RESULTS_DIR/cachegrind_annotated.txt"
fi

echo "== Ejecutando profiling de memoria con Massif =="
valgrind --tool=massif \
    --massif-out-file="$RESULTS_DIR/massif.out" \
    "$BIN" "$SIZE" > "$RESULTS_DIR/massif_stdout.txt" 2>&1

if command -v ms_print >/dev/null 2>&1; then
    ms_print "$RESULTS_DIR/massif.out" > "$RESULTS_DIR/massif_report.txt"
fi

echo "== Ejecutando profiling de memoria =="
valgrind --tool=memcheck \
    --leak-check=full \
    --show-leak-kinds=all \
    --track-origins=yes \
    --log-file="$RESULTS_DIR/memcheck.txt" \
    "$BIN" "$SIZE" > "$RESULTS_DIR/memcheck_stdout.txt" 2>&1

echo "== Resumen de CPU real =="
/usr/bin/time -v "$BIN" "$SIZE" > "$RESULTS_DIR/time_stdout.txt" 2> "$RESULTS_DIR/time_report.txt"

echo "== Ejecutando profiling con gprof =="
rm -f "$SCRIPT_DIR/gmon.out"
"$GPROF_BIN" "$SIZE" > "$RESULTS_DIR/gprof_stdout.txt" 2>&1

if [[ -f "$SCRIPT_DIR/gmon.out" ]]; then
    mv "$SCRIPT_DIR/gmon.out" "$RESULTS_DIR/gmon.out"
    gprof "$GPROF_BIN" "$RESULTS_DIR/gmon.out" > "$RESULTS_DIR/gprof_report.txt"
fi

cat > "$RESULTS_DIR/README.txt" <<EOF
Profiling generado para matrix_multiply_openmp.c

Tamaño de matriz: $SIZE
Hilos OpenMP: $THREADS

Archivos generados:
- callgrind.out / callgrind_stdout.txt / callgrind_annotated.txt
- cachegrind.out / cachegrind_stdout.txt / cachegrind_annotated.txt
- massif.out / massif_stdout.txt / massif_report.txt
- memcheck.txt / memcheck_stdout.txt
- time_stdout.txt / time_report.txt
- gprof_stdout.txt / gmon.out / gprof_report.txt

Notas:
- Callgrind sirve como profiling de CPU a nivel de instrucciones.
- Cachegrind sirve para estudiar el comportamiento de caché.
- Massif sirve para ver el uso de heap y el pico de memoria.
- Memcheck sirve para fugas y errores de memoria.
- gprof sirve para hotspots por función, aunque en OpenMP puede ser menos preciso.
- Para Valgrind, conviene usar tamaños pequeños o medianos.
EOF

echo ""
echo "Profiling terminado. Resultados en: $RESULTS_DIR"
