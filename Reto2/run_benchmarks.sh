#!/bin/bash

SERIAL="./reto2"
OPENMP="./reto2Concurrent1"
MPI="./reto2Concurrent2"

# Archivos de salida individuales
OUT_SERIAL="results_serial.csv"
OUT_OPENMP="resultadoOpenmpMPI.csv"
OUT_MPI="results_mpi.csv"


# Configuración
MPI_RANKS=4
THREADS=4
RUNS=10

# Compilación
echo "Compiling..."
gcc -O3 -std=c11 reto2.c -o "$SERIAL"
gcc -O3 -fopenmp -std=c11 reto2Concurrent1.c -o "$OPENMP"
mpicc -O3 -std=c11 reto2Concurrent2.c -o "$MPI"

# --- INICIALIZACIÓN INTELIGENTE ---
# Solo escribimos el encabezado si el archivo NO existe
HEADER="cells,steps,density,threads_or_ranks,wall_time"

for f in "$OUT_SERIAL" "$OUT_OPENMP" "$OUT_MPI"; do
    if [ ! -f "$f" ]; then
        echo "$HEADER" > "$f"
        echo "Created $f with header."
    else
        echo "Appending to existing $f."
    fi
done

execute_run() {
    local type=$1
    local c=$2
    local s=$3
    local d=$4
    local out_line=""

    case $type in
        "Serial")
            out_line=$("$SERIAL" "$c" "$s" "$d" 1 | tail -n 1)
            echo "$out_line" >> "$OUT_SERIAL"
            ;;
        "OpenMP")
            out_line=$(OMP_NUM_THREADS=$THREADS "$OPENMP" "$c" "$s" "$d" 1 | tail -n 1)
            echo "$out_line" >> "$OUT_OPENMP"
            ;;
        "MPI")
            out_line=$(mpirun -np $MPI_RANKS "$MPI" "$c" "$s" "$d" 1 | tail -n 1)
            echo "$out_line" >> "$OUT_MPI"
            ;;
    esac
}

# --- BUCLE DE PRUEBAS INTERCALADAS ---


CELL_SIZES_VARY=(5000000 10000000 15000000 20000000)
ITERATIONS_VARY=(1000 2000 3000 4000 5000 )

for c in "${CELL_SIZES_VARY[@]}"; do
    for s in "${ITERATIONS_VARY[@]}"; do
        echo "--- Testing Workload: $c cells, $s steps ---"
        for i in $(seq 1 $RUNS); do
            echo "  Iteration $i/$RUNS..."
            #execute_run "Serial" "$c" "$s" 0.3
            execute_run "OpenMP" "$c" "$s" 0.3
            execute_run "MPI" "$c" "$s" 0.3
        done
    done
done




echo ""
echo "=== DONE ==="
echo "Data appended to the CSV files."

sudo shutdown now
