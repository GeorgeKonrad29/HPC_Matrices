#!/usr/bin/env python3
"""Genera mapas de calor (heatmaps) de speedup para 3 versiones paralelas.

Cada mapa tiene el eje X = `cells`, eje Y = `steps` (iteraciones) y el tono
de color representa el speedup relativo al serial (serial / paralelo).

Guarda una figura con los 3 heatmaps en `speedup_heatmaps.png` y archivos
por separado `speedup_<version>.png`.
"""
import os
import sys
import numpy as np
import pandas as pd
import matplotlib.pyplot as plt

BASE_DIR = os.path.dirname(os.path.abspath(__file__))

SERIAL_FILE = 'results_serial.csv'
PARALLEL_FILES = {
    'MPI': 'results_mpi.csv',
    'OpenMP Convencional': 'resultadoOpenmpConvencional.csv',
    'OpenMP+MPI': 'resultadoOpenmpMPI.csv',
}

CSV_COLS = ['cells', 'steps', 'density', 'threads_or_ranks', 'wall_time']


def load_clean_csv(path):
    try:
        df = pd.read_csv(path, header=0)
        if list(df.columns) != CSV_COLS:
            df = pd.read_csv(path, header=None, names=CSV_COLS)
    except Exception:
        df = pd.read_csv(path, header=None, names=CSV_COLS)

    for c in CSV_COLS:
        df[c] = pd.to_numeric(df[c], errors='coerce')
    df = df.dropna(subset=['cells', 'steps', 'wall_time'])
    df['cells'] = df['cells'].astype(int)
    df['steps'] = df['steps'].astype(int)
    df['threads_or_ranks'] = df['threads_or_ranks'].astype(int)
    return df


def pivot_median(df):
    """Devuelve un DataFrame pivot con filas=steps, columnas=cells y valores=mediana wall_time."""
    med = df.groupby(['steps', 'cells'])['wall_time'].median()
    pivot = med.unstack(level='cells')
    pivot = pivot.sort_index(axis=0)  # ordenar steps
    pivot = pivot.reindex(sorted(pivot.columns), axis=1)
    return pivot


def sanitize_label(label):
    return ''.join(ch if ch.isalnum() else '_' for ch in label)


def main():
    serial_path = os.path.join(BASE_DIR, SERIAL_FILE)
    if not os.path.exists(serial_path):
        print(f'Error: no se encontró {serial_path}')
        sys.exit(1)

    serial_df = load_clean_csv(serial_path)
    serial_pivot = pivot_median(serial_df)
    cells = list(serial_pivot.columns)
    steps = list(serial_pivot.index)

    speedup_dict = {}
    for label, fname in PARALLEL_FILES.items():
        path = os.path.join(BASE_DIR, fname)
        if not os.path.exists(path):
            print(f'Aviso: {path} no existe — se salta {label}')
            continue
        par_df = load_clean_csv(path)
        par_pivot = pivot_median(par_df)
        # alineamos a la rejilla del serial (mismas filas y columnas)
        par_pivot = par_pivot.reindex(index=serial_pivot.index, columns=serial_pivot.columns)
        par_pivot = par_pivot.replace(0, np.nan)
        sp = serial_pivot / par_pivot
        speedup_dict[label] = sp

    if not speedup_dict:
        print('No se encontraron versiones paralelas para graficar.')
        sys.exit(0)

    # escala de color compartida
    all_vals = np.stack([s.values for s in speedup_dict.values()])
    try:
        vmin = float(np.nanmin(all_vals))
        vmax = float(np.nanmax(all_vals))
    except ValueError:
        print('No hay valores válidos para speedup.')
        sys.exit(0)

    n = len(speedup_dict)
    fig, axes = plt.subplots(1, n, figsize=(5 * n, 5))
    if n == 1:
        axes = [axes]

    for ax, (label, sp_df) in zip(axes, speedup_dict.items()):
        data = sp_df.values
        im = ax.imshow(data, aspect='auto', origin='lower', interpolation='nearest', cmap='viridis', vmin=vmin, vmax=vmax)
        ax.set_xticks(np.arange(len(cells)))
        ax.set_xticklabels([str(int(c)) for c in cells], rotation=45, ha='right')
        ax.set_yticks(np.arange(len(steps)))
        ax.set_yticklabels([str(int(s)) for s in steps])
        ax.set_xlabel('cells')
        ax.set_ylabel('steps')
        ax.set_title(label)

    cbar = fig.colorbar(im, ax=list(axes), orientation='vertical', fraction=0.025, pad=0.04)
    cbar.set_label('Speedup (serial / paralelo)')
    fig.tight_layout()

    out_path = os.path.join(BASE_DIR, 'speedup_heatmaps.png')
    fig.savefig(out_path, dpi=200)
    print(f'Guardado: {out_path}')

    # Guardar imágenes individuales también
    for label, sp_df in speedup_dict.items():
        fig2, ax2 = plt.subplots(figsize=(6, 5))
        im2 = ax2.imshow(sp_df.values, aspect='auto', origin='lower', interpolation='nearest', cmap='viridis', vmin=vmin, vmax=vmax)
        ax2.set_xticks(np.arange(len(cells)))
        ax2.set_xticklabels([str(int(c)) for c in cells], rotation=45, ha='right')
        ax2.set_yticks(np.arange(len(steps)))
        ax2.set_yticklabels([str(int(s)) for s in steps])
        ax2.set_xlabel('cells')
        ax2.set_ylabel('steps')
        ax2.set_title(label)
        cbar2 = fig2.colorbar(im2, ax=ax2)
        cbar2.set_label('Speedup (serial / paralelo)')
        fig2.tight_layout()
        safe = sanitize_label(label)
        out2 = os.path.join(BASE_DIR, f'speedup_{safe}.png')
        fig2.savefig(out2, dpi=200)
        print(f'Guardado: {out2}')
        plt.close(fig2)

    plt.close('all')


if __name__ == '__main__':
    main()
