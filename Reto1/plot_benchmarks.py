#!/usr/bin/env python3

import csv
import os
from collections import defaultdict
from statistics import mean, stdev

import matplotlib.pyplot as plt


METRIC_LABELS = {
    "time_seconds": "Tiempo (s)",
    "iterations": "Iteraciones",
    "error_jacobi": "Error RMS Jacobi",
    "error_discretization": "Error RMS Discretización",
}

TEST_TYPE_LABELS = {
    "secuencial_std": "Secuencial",
    "secuencial_ofast": "Secuencial -Ofast",
    "threads": "Threads",
    "processes": "Procesos",
}

PREFERRED_INPUTS = [
    "benchmark_results_v2.csv",
    "benchmark_result_tolerances.csv",
    "benchmark_results_tolerance.csv",
    "benchmark_results.csv",
]


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


def read_rows(csv_path):
    rows = []
    with open(csv_path, "r", newline="") as file:
        reader = csv.DictReader(file)
        for row in reader:
            normalized = {
                "phase": row.get("phase", "").strip(),
                "run": safe_int(row.get("run", 0), 0),
                "test_type": row.get("test_type", "").strip(),
                "k": safe_int(row.get("k", 0), 0),
                "tolerance": safe_float(row.get("tolerance", 0.0), 0.0),
                "nthreads": safe_int(row.get("nthreads", 0), 0),
                "nproc": safe_int(row.get("nproc", 0), 0),
                "iterations": safe_int(row.get("iterations", 0), 0),
                "time_seconds": safe_float(row.get("time_seconds", 0.0), 0.0),
                "error_jacobi": safe_float(row.get("error_jacobi", 0.0), 0.0),
                "error_discretization": safe_float(row.get("error_discretization", 0.0), 0.0),
            }
            if normalized["phase"] and normalized["test_type"]:
                rows.append(normalized)
    return rows


def collect_rows(input_files):
    all_rows = []
    for file_path in input_files:
        all_rows.extend(read_rows(file_path))
    return all_rows


def metric_stats(rows, phase, x_key, metric):
    grouped = defaultdict(list)
    for row in rows:
        if row["phase"] != phase:
            continue
        grouped[(row["test_type"], row[x_key])].append(row[metric])

    stats = defaultdict(dict)
    for (test_type, x_value), values in grouped.items():
        if not values:
            continue
        avg = mean(values)
        spread = stdev(values) if len(values) > 1 else 0.0
        stats[test_type][x_value] = (avg, spread)
    return stats


def speedup_stats(rows, phase, x_key, baseline_test_type="secuencial_std"):
    base_times = defaultdict(list)
    test_times = defaultdict(list)

    for row in rows:
        if row["phase"] != phase:
            continue
        x_value = row[x_key]
        if row["test_type"] == baseline_test_type:
            base_times[x_value].append(row["time_seconds"])
        else:
            test_times[(row["test_type"], x_value)].append(row["time_seconds"])

    speedups = defaultdict(dict)
    for (test_type, x_value), values in test_times.items():
        if x_value not in base_times or not values:
            continue
        baseline = mean(base_times[x_value])
        current = mean(values)
        if current <= 0:
            continue
        speedups[test_type][x_value] = baseline / current

    return speedups


def phase_x_values(rows, phase, x_key):
    values = sorted({row[x_key] for row in rows if row["phase"] == phase})
    if x_key == "tolerance":
        values = sorted(values, reverse=True)
    return values


def human_test_type(test_type):
    return TEST_TYPE_LABELS.get(test_type, test_type)


def make_plot_title(phase, metric_name):
    phase_label = "Barrido K" if phase == "k_sweep" else "Barrido Tolerancia"
    return f"{metric_name} - {phase_label}"


def x_axis_label(phase):
    return "K" if phase == "k_sweep" else "Tolerancia"


def x_value_label(phase, value):
    if phase == "tol_sweep":
        return f"{value:.0e}"
    return str(value)


def plot_metric(rows, phase, x_key, metric, output_dir, output_suffix=None):
    stats = metric_stats(rows, phase, x_key, metric)
    x_values = phase_x_values(rows, phase, x_key)
    if not stats or not x_values:
        return None

    plt.figure(figsize=(10, 6))
    for test_type in sorted(stats.keys()):
        xs = []
        ys = []
        yerr = []
        for x_value in x_values:
            values = stats[test_type].get(x_value)
            if values is None:
                continue
            xs.append(x_value)
            ys.append(values[0])
            yerr.append(values[1])

        if xs:
            plt.errorbar(xs, ys, yerr=yerr, marker="o", capsize=4, linewidth=1.8, label=human_test_type(test_type))

    if metric in {"error_jacobi", "error_discretization"}:
        plt.yscale("log")

    plt.xlabel(x_axis_label(phase))
    plt.ylabel(METRIC_LABELS[metric])
    plt.title(make_plot_title(phase, METRIC_LABELS[metric]))
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.xticks(x_values, [x_value_label(phase, value) for value in x_values])
    plt.tight_layout()

    suffix = output_suffix if output_suffix else metric
    output_path = os.path.join(output_dir, f"{phase}_{suffix}.png")
    plt.savefig(output_path, dpi=300)
    plt.close()
    return output_path


def plot_time_comparison(rows, phase, x_key, output_dir):
    return plot_metric(rows, phase, x_key, "time_seconds", output_dir, output_suffix="time_comparison")


def plot_speedup(rows, phase, x_key, output_dir):
    speedups = speedup_stats(rows, phase, x_key)
    x_values = phase_x_values(rows, phase, x_key)
    if not speedups or not x_values:
        return None

    plt.figure(figsize=(10, 6))
    for test_type in sorted(speedups.keys()):
        xs = []
        ys = []
        for x_value in x_values:
            value = speedups[test_type].get(x_value)
            if value is None:
                continue
            xs.append(x_value)
            ys.append(value)
        if xs:
            plt.plot(xs, ys, marker="o", linewidth=2, label=human_test_type(test_type))

    plt.axhline(y=1.0, linestyle="--", linewidth=1, color="black", alpha=0.6, label="Referencia x1")
    plt.xlabel(x_axis_label(phase))
    plt.ylabel("Speedup (vs Secuencial)")
    phase_label = "Barrido K" if phase == "k_sweep" else "Barrido Tolerancia"
    plt.title(f"Speedup - {phase_label}")
    plt.grid(True, alpha=0.3)
    plt.legend()
    plt.xticks(x_values, [x_value_label(phase, value) for value in x_values])
    plt.tight_layout()

    output_path = os.path.join(output_dir, f"{phase}_speedup.png")
    plt.savefig(output_path, dpi=300)
    plt.close()
    return output_path


def discover_input_files(base_dir):
    selected = []
    for name in PREFERRED_INPUTS:
        path = os.path.join(base_dir, name)
        if os.path.isfile(path):
            selected.append(path)
    return selected


def main():
    base_dir = os.path.dirname(os.path.abspath(__file__))
    output_dir = os.path.join(base_dir, "plots")
    os.makedirs(output_dir, exist_ok=True)

    input_files = discover_input_files(base_dir)
    if not input_files:
        print("No se encontraron archivos CSV de benchmark.")
        print("Busca alguno de estos nombres en el directorio actual:")
        for file_name in PREFERRED_INPUTS:
            print(f"  - {file_name}")
        return

    rows = collect_rows(input_files)
    if not rows:
        print("No hay filas válidas para graficar.")
        return

    print("Archivos leídos:")
    for csv_file in input_files:
        print(f"  - {os.path.basename(csv_file)}")

    generated = []
    phase_specs = [("k_sweep", "k"), ("tol_sweep", "tolerance")]
    for phase, x_key in phase_specs:
        if not any(row["phase"] == phase for row in rows):
            continue

        time_output = plot_time_comparison(rows, phase, x_key, output_dir)
        if time_output:
            generated.append(time_output)

        speedup_output = plot_speedup(rows, phase, x_key, output_dir)
        if speedup_output:
            generated.append(speedup_output)

    if not generated:
        print("No se generaron gráficas. Revisa las fases y columnas del CSV.")
        return

    print("\nGráficas generadas:")
    for path in generated:
        print(f"  - {os.path.basename(path)}")
    print(f"\nDirectorio de salida: {output_dir}")


if __name__ == "__main__":
    main()
