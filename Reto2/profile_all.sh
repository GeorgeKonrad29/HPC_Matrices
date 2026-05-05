#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
OUTDIR="$ROOT/profiles"
mkdir -p "$OUTDIR"

# Parámetros: CELLS STEPS DENSITY OMP_THREADS MPI_RANKS REPEATS VALGRIND_CELLS VALGRIND_STEPS
CELLS=${1:-10000000}
STEPS=${2:-1000}
DENSITY=${3:-0.3}
OMP_THREADS=${4:-4}
MPI_RANKS=${5:-4}
REPEATS=${6:-5}
VALGRIND_CELLS=${7:-1000000}
VALGRIND_STEPS=${8:-200}

CFLAGS="-O3 -march=native"

command_exists() { command -v "$1" >/dev/null 2>&1; }

echo "Output -> $OUTDIR"
echo "Config: CELLS=$CELLS STEPS=$STEPS DENSITY=$DENSITY OMP_THREADS=$OMP_THREADS MPI_RANKS=$MPI_RANKS"

# Opcional: compilar si hay fuentes
if [ -f "$ROOT/reto2.c" ]; then
  echo "Compiling reto2..."
  gcc $CFLAGS "$ROOT/reto2.c" -o "$ROOT/reto2" || true
fi
if [ -f "$ROOT/reto2Concurrent1.c" ]; then
  echo "Compiling reto2Concurrent1..."
  gcc $CFLAGS -fopenmp "$ROOT/reto2Concurrent1.c" -o "$ROOT/reto2Concurrent1" || true
fi
if [ -f "$ROOT/reto2Concurrent3.c" ]; then
  echo "Compiling reto2Concurrent3..."
  gcc $CFLAGS -fopenmp "$ROOT/reto2Concurrent3.c" -o "$ROOT/reto2Concurrent3" || true
fi
if [ -f "$ROOT/reto2Concurrent2.c" ] && command_exists mpicc; then
  echo "Compiling reto2Concurrent2 (MPI)..."
  mpicc $CFLAGS "$ROOT/reto2Concurrent2.c" -o "$ROOT/reto2Concurrent2" || true
fi

run_perf_stat() {
  local name=$1; shift
  local out="$OUTDIR/perf_stat_${name}.txt"
  echo "perf stat ($name) -> $out"
  perf stat -r $REPEATS -o "$out" -- "$@" 2>&1 || true
}

run_perf_record() {
  local name=$1; shift
  local data="$OUTDIR/perf_record_${name}.data"
  local report="$OUTDIR/perf_report_${name}.txt"
  echo "perf record ($name) -> $data"
  perf record -F 99 -g -o "$data" -- "$@" 2>/dev/null || true
  echo "perf report -> $report"
  perf report -i "$data" --stdio > "$report" 2>/dev/null || true
}

run_cachegrind() {
  local name=$1; shift
  local out="$OUTDIR/cachegrind_${name}.out"
  local txt="$OUTDIR/cachegrind_${name}.txt"
  echo "cachegrind ($name) -> $out"
  OMP_NUM_THREADS=1 valgrind --tool=cachegrind --cachegrind-out-file="$out" -- "$@" >/dev/null 2>&1 || true
  if command_exists cg_annotate; then
    cg_annotate "$out" > "$txt" || true
  fi
}

bins=( "reto2" "reto2Concurrent1" "reto2Concurrent3" "reto2Concurrent2" )

for b in "${bins[@]}"; do
  BIN="$ROOT/$b"
  if [ ! -x "$BIN" ]; then
    echo "Skipping $b (no existe o no ejecutable)"
    continue
  fi

  echo "=== Profiling $b ==="

  if [ "$b" = "reto2Concurrent2" ]; then
    base_cmd=( mpirun -np "$MPI_RANKS" "$BIN" "$CELLS" "$STEPS" "$DENSITY" 1 )
  else
    base_cmd=( "$BIN" "$CELLS" "$STEPS" "$DENSITY" 1 )
  fi

  # perf estadístico (multi-run)
  if [ "$b" != "reto2" ] && [ "$b" != "reto2Concurrent2" ]; then
    # OpenMP: exportar OMP_NUM_THREADS
    run_perf_stat "${b}" env OMP_NUM_THREADS="$OMP_THREADS" "${base_cmd[@]}"
    run_perf_record "${b}" env OMP_NUM_THREADS="$OMP_THREADS" "${base_cmd[@]}"
  else
    run_perf_stat "${b}" "${base_cmd[@]}"
    run_perf_record "${b}" "${base_cmd[@]}"
  fi

  # Cachegrind (pequeña entrada) — lento, OMP en 1 hilo
  if [ "$b" = "reto2Concurrent2" ]; then
    echo "Skipping valgrind/cachegrind for MPI binary $b (use manual run if needed)"
  else
    small_cmd=( "$BIN" "$VALGRIND_CELLS" "$VALGRIND_STEPS" "$DENSITY" 1 )
    run_cachegrind "${b}" "${small_cmd[@]}"
  fi

done

echo "Done. Result files in: $OUTDIR"