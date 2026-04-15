#!/usr/bin/env python3

import argparse
import csv
import os
from collections import defaultdict
from statistics import mean

import matplotlib.pyplot as plt


LABELS = {
    "secuencial_std": "Secuencial",
    "secuencial_ofast": "Secuencial -Ofast",
    "threads": "Threads",
    "processes": "Procesos",
}


def safe_float(value, default=0.0):
    try:
        return float(value)
    except (TypeError, ValueError):
        return default


def safe_int(value, default=0):
    try:
        return int(float(value))
    except (TypeError, ValueError):
        return default


def read_k_sweep_rows(csv_path):
    rows = []
    with open(csv_path, "r", newline="") as file:
        reader = csv.DictReader(file)
        for row in reader:
            phase = (row.get("phase") or "").strip()
            if phase != "k_sweep":
                continue
            rows.append(
                {
                    "test_type": (row.get("test_type") or "").strip(),
                    "k": safe_int(row.get("k"), 0),
                    "time_seconds": safe_float(row.get("time_seconds"), 0.0),
                }
            )
    return rows


def aggregate_times(rows):
    grouped = defaultdict(list)
    for row in rows:
        grouped[(row["test_type"], row["k"])].append(row["time_seconds"])

    aggregated = defaultdict(dict)
    for (test_type, k_value), values in grouped.items():
        if values:
            aggregated[test_type][k_value] = mean(values)
    return aggregated


def compute_speedup(aggregated, baseline="secuencial_std"):
    speedup = defaultdict(dict)
    base = aggregated.get(baseline, {})

    for test_type, by_k in aggregated.items():
        if test_type == baseline:
            continue
        for k_value, time_value in by_k.items():
            base_time = base.get(k_value)
            if base_time is None or time_value <= 0:
                continue
            speedup[test_type][k_value] = base_time / time_value
    return speedup


def display_label(test_type):
    return LABELS.get(test_type, test_type)


def plot_time_comparison(aggregated, output_path):
    all_k = sorted({k for by_k in aggregated.values() for k in by_k.keys()})
    if not all_k:
        return None

    plt.figure(figsize=(10, 6))
    for test_type in sorted(aggregated.keys()):
        xs = []
        ys = []
        for k_value in all_k:
            value = aggregated[test_type].get(k_value)
            if value is None:
                continue
            xs.append(k_value)
            ys.append(value)
        if xs:
            plt.plot(xs, ys, marker="o", linewidth=2, label=display_label(test_type))

    plt.title("Comparación de tiempos (k_sweep)")
    plt.xlabel("K")
    plt.ylabel("Tiempo promedio (s)")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    plt.close()
    return output_path


def plot_speedup(speedup_data, output_path):
    all_k = sorted({k for by_k in speedup_data.values() for k in by_k.keys()})
    if not all_k:
        return None

    plt.figure(figsize=(10, 6))
    for test_type in sorted(speedup_data.keys()):
        xs = []
        ys = []
        for k_value in all_k:
            value = speedup_data[test_type].get(k_value)
            if value is None:
                continue
            xs.append(k_value)
            ys.append(value)
        if xs:
            plt.plot(xs, ys, marker="o", linewidth=2, label=display_label(test_type))

    plt.axhline(1.0, linestyle="--", linewidth=1, color="black", alpha=0.6, label="Secuencial x1")
    plt.title("Speedup vs Secuencial (k_sweep)")
    plt.xlabel("K")
    plt.ylabel("Speedup")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.tight_layout()
    plt.savefig(output_path, dpi=300)
    plt.close()
    return output_path


def main():
    parser = argparse.ArgumentParser(description="Genera speedup y comparación de tiempos para k_sweep.")
    parser.add_argument("--input", default="benchmark_results.csv", help="CSV de entrada")
    parser.add_argument("--outdir", default="plots", help="Directorio de salida")
    args = parser.parse_args()

    input_path = os.path.abspath(args.input)
    outdir = os.path.abspath(args.outdir)
    os.makedirs(outdir, exist_ok=True)

    rows = read_k_sweep_rows(input_path)
    if not rows:
        print("No se encontraron datos con phase=k_sweep en el CSV.")
        return

    aggregated = aggregate_times(rows)
    speedup_data = compute_speedup(aggregated, baseline="secuencial_std")

    time_file = os.path.join(outdir, "time_comparison_vs_k.png")
    speedup_file = os.path.join(outdir, "speedup_vs_k.png")

    generated_time = plot_time_comparison(aggregated, time_file)
    generated_speedup = plot_speedup(speedup_data, speedup_file)

    if generated_time:
        print(f"Generada: {generated_time}")
    if generated_speedup:
        print(f"Generada: {generated_speedup}")


if __name__ == "__main__":
    main()
