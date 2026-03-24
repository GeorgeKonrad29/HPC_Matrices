#!/usr/bin/env python3

import csv
import statistics
from collections import defaultdict
from pathlib import Path


def escape_latex(text: str) -> str:
    replacements = {
        "_": r"\_",
        "%": r"\%",
        "&": r"\&",
        "#": r"\#",
        "$": r"\$",
    }
    output = text
    for old, new in replacements.items():
        output = output.replace(old, new)
    return output


def load_data(csv_path: Path):
    data = defaultdict(lambda: defaultdict(list))
    with csv_path.open("r", newline="", encoding="utf-8") as file:
        reader = csv.DictReader(file)
        for row in reader:
            file_name = row["Archivo"].strip()
            size = int(row["Tamaño"].split("x")[0])
            time_s = float(row["Tiempo (s)"])
            data[file_name][size].append(time_s)
    return data


def summarize(data):
    summary = {}
    for file_name, sizes in data.items():
        summary[file_name] = {}
        for size, values in sorted(sizes.items()):
            mean_v = statistics.fmean(values)
            std_v = statistics.stdev(values) if len(values) > 1 else 0.0
            summary[file_name][size] = {
                "n": len(values),
                "mean": mean_v,
                "std": std_v,
                "min": min(values),
                "max": max(values),
            }
    return summary


def speedup_against_standard(summary, baseline_name="standar.doc"):
    if baseline_name not in summary:
        return {}, {}

    baseline = summary[baseline_name]
    speedups = defaultdict(dict)

    for file_name, sizes in summary.items():
        if file_name == baseline_name:
            continue
        for size, stats in sizes.items():
            if size in baseline and stats["mean"] > 0:
                speedups[file_name][size] = baseline[size]["mean"] / stats["mean"]

    avg_speedup = {}
    for file_name, sizes in speedups.items():
        if sizes:
            avg_speedup[file_name] = statistics.fmean(list(sizes.values()))

    return speedups, avg_speedup


def best_by_size(summary, baseline_name="standar.doc"):
    all_sizes = sorted({size for sizes in summary.values() for size in sizes.keys()})
    winners = []
    for size in all_sizes:
        candidates = []
        for file_name, sizes in summary.items():
            if file_name == baseline_name:
                continue
            if size in sizes:
                candidates.append((file_name, sizes[size]["mean"]))
        if candidates:
            winner = min(candidates, key=lambda item: item[1])
            winners.append((size, winner[0], winner[1]))
    return winners


def pretty_name(file_name: str) -> str:
    base = file_name.replace(".doc", "")
    return base.replace("hilos", " hilos").replace("procesos", " procesos")


def generate_tex(summary, output_path: Path):
    sorted_files = sorted(summary.keys())

    lines = []
    lines.append(r"\documentclass[conference]{IEEEtran}")
    lines.append(r"\IEEEoverridecommandlockouts")
    lines.append(r"\usepackage[utf8]{inputenc}")
    lines.append(r"\usepackage[T1]{fontenc}")
    lines.append(r"\usepackage[spanish]{babel}")
    lines.append(r"\usepackage{geometry}")
    lines.append(r"\usepackage{graphicx}")
    lines.append(r"\usepackage{booktabs}")
    lines.append(r"\usepackage{longtable}")
    lines.append(r"\usepackage{array}")
    lines.append(r"\usepackage{caption}")
    lines.append(r"\usepackage{setspace}")
    lines.append(r"\geometry{margin=2.5cm}")
    lines.append("")
    lines.append(r"\begin{document}")

    lines.append(r"\begin{titlepage}")
    lines.append(r"\centering")
    lines.append(r"{\Large \textbf{INSTITUCIÓN / UNIVERSIDAD}}\\[0.8cm]")
    lines.append(r"{\large FACULTAD / ESCUELA}\\[0.5cm]")
    lines.append(r"{\large PROGRAMA / CARRERA}\\[2.0cm]")
    lines.append(r"{\LARGE \textbf{Análisis de Rendimiento de Multiplicación de Matrices}}\\[0.8cm]")
    lines.append(r"{\large (Formato IEEE)}\\[2.0cm]")
    lines.append(r"\begin{flushleft}")
    lines.append(r"\textbf{Autor(es):} \rule{10cm}{0.4pt}\\[0.4cm]")
    lines.append(r"\textbf{Código / ID:} \rule{10cm}{0.4pt}\\[0.4cm]")
    lines.append(r"\textbf{Asignatura:} \rule{10cm}{0.4pt}\\[0.4cm]")
    lines.append(r"\textbf{Docente:} \rule{10cm}{0.4pt}\\[0.4cm]")
    lines.append(r"\textbf{Fecha:} \rule{10cm}{0.4pt}")
    lines.append(r"\end{flushleft}")
    lines.append(r"\vfill")
    lines.append(r"{\large CIUDAD -- PAÍS}")
    lines.append(r"\end{titlepage}")

    lines.append(r"\tableofcontents")
    lines.append(r"\clearpage")
    lines.append(r"\onecolumn")

    lines.append(r"\section{Objetivo}")
    lines.append(
        r"Este informe presenta el análisis de tiempo de ejecución y speedup para las variantes "
        r"de multiplicación de matrices incluidas en el archivo de datos \texttt{resultados.csv}. "
        r"Las gráficas se incluyen como evidencia visual del comportamiento observado."
    )

    lines.append(r"\section{Marco Conceptual}")
    lines.append(
        r"La métrica de \textit{wall-clock time} corresponde al tiempo real transcurrido durante la ejecución "
        r"de un algoritmo. En este estudio se compara dicha métrica entre implementaciones secuenciales y "
        r"paralelas (hilos y procesos) para multiplicación de matrices cuadradas."
    )
    lines.append(
        r"El \textit{speedup} se define como $S = T_{base} / T_{paralelo}$, donde $T_{base}$ es el tiempo de la "
        r"implementación de referencia (\texttt{standar.doc}) y $T_{paralelo}$ el tiempo promedio de la variante "
        r"comparada para un mismo tamaño de matriz."
    )
    lines.append(
        r"Dado que el costo de la multiplicación clásica de matrices crece aproximadamente como $O(n^3)$, "
        r"se espera un incremento pronunciado en el tiempo al aumentar el tamaño de entrada."
    )

    lines.append(r"\section{Marco Contextual}")
    lines.append(r"\textbf{Institución:} \rule{12cm}{0.4pt}\\[0.3cm]")
    lines.append(r"\textbf{Asignatura:} \rule{12cm}{0.4pt}\\[0.3cm]")
    lines.append(r"\textbf{Docente:} \rule{12cm}{0.4pt}\\[0.3cm]")
    lines.append(r"\textbf{Semestre / Periodo:} \rule{12cm}{0.4pt}\\[0.3cm]")
    lines.append(r"\textbf{Entorno de ejecución (hardware/software):} \rule{8cm}{0.4pt}\\[0.3cm]")
    lines.append(r"\textbf{Objetivo particular del experimento:} \\[-0.1cm]")
    lines.append(r"\rule{\textwidth}{0.4pt}\\[0.2cm]")
    lines.append(r"\rule{\textwidth}{0.4pt}\\[0.2cm]")
    lines.append(r"\rule{\textwidth}{0.4pt}")

    lines.append(r"\section{Gráficas}")
    lines.append(r"\begin{figure}[htbp]")
    lines.append(r"\centering")
    lines.append(r"\includegraphics[width=\textwidth]{speedup_graph.png}")
    lines.append(r"\caption{Speedup respecto a la versión estándar (standar.doc).}")
    lines.append(r"\label{fig:speedup}")
    lines.append(r"\end{figure}")

    lines.append(r"\begin{figure}[htbp]")
    lines.append(r"\centering")
    lines.append(r"\includegraphics[width=\textwidth]{execution_time_graph.png}")
    lines.append(r"\caption{Tiempo de ejecución promedio vs tamaño de matriz.}")
    lines.append(r"\label{fig:time}")
    lines.append(r"\end{figure}")

    lines.append(r"\clearpage")
    lines.append(r"\section{Tablas por archivo}")
    lines.append(
        r"Cada tabla contiene métricas descriptivas por tamaño: número de ejecuciones ($n$), "
        r"promedio, desviación estándar, mínimo y máximo."
    )

    for file_name in sorted_files:
        lines.append(rf"\subsection{{{escape_latex(file_name)}}}")
        lines.append(r"\begin{center}")
        lines.append(r"\begin{tabular}{rrrrrr}")
        lines.append(r"\toprule")
        lines.append(r"Tamaño & $n$ & Promedio (s) & Desv. est. (s) & Mínimo (s) & Máximo (s) \\")
        lines.append(r"\midrule")

        for size in sorted(summary[file_name].keys()):
            stats = summary[file_name][size]
            lines.append(
                f"{size} & {stats['n']} & {stats['mean']:.4f} & {stats['std']:.4f} & {stats['min']:.4f} & {stats['max']:.4f} \\\\"
            )

        lines.append(r"\bottomrule")
        lines.append(r"\end{tabular}")
        lines.append(r"\end{center}")

    lines.append(r"\section{Conclusiones}")
    lines.append(r"\textit{Completar esta sección con las conclusiones del análisis.}\\[0.2cm]")
    lines.append(r"\begin{itemize}")
    lines.append(r"\item [Conclusión 1] ")
    lines.append(r"\item [Conclusión 2] ")
    lines.append(r"\item [Conclusión 3] ")
    lines.append(r"\end{itemize}")

    lines.append(r"\section{Trabajo Futuro}")
    lines.append(r"\textit{Opcional: incluir mejoras propuestas, nuevas métricas o escenarios de prueba adicionales.}")

    lines.append(r"\end{document}")

    output_path.write_text("\n".join(lines), encoding="utf-8")


def main():
    script_dir = Path(__file__).parent
    csv_path = script_dir / "resultados.csv"
    output_path = script_dir / "reporte_overleaf.tex"

    if not csv_path.exists():
        raise FileNotFoundError(f"No existe {csv_path}")

    data = load_data(csv_path)
    summary = summarize(data)
    generate_tex(summary, output_path)
    print(f"Archivo generado: {output_path}")


if __name__ == "__main__":
    main()
