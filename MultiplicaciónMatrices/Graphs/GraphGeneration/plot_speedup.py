#!/usr/bin/env python3

import re
import csv
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict
from pathlib import Path

def extract_size_value(size_str):
    """Extrae el valor numérico del tamaño (ej: '1000x1000' -> 1000)"""
    match = re.search(r'(\d+)x\d+', size_str)
    if match:
        return int(match.group(1))
    return 0

def extract_standard_data():
    """Extrae los datos estándar del CSV"""
    standard_data = {}
    
    script_dir = Path(__file__).parent
    csv_path = script_dir / 'resultados.csv'
    try:
        with open(csv_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                if row['Archivo'] == 'standar.doc':
                    tamaño = row['Tamaño'].split('x')[0]  # Ej: "1000x1000" -> "1000"
                    tiempo = float(row['Tiempo (s)'])
                    
                    if tamaño not in standard_data:
                        standard_data[tamaño] = []
                    standard_data[tamaño].append(tiempo)
    except Exception as e:
        print(f"Error leyendo standar.doc del CSV: {e}")
        return standard_data
    
    # Promediar los tiempos para cada tamaño
    for size in standard_data:
        standard_data[size] = np.mean(standard_data[size])
    
    return standard_data

def calculate_speedup(csv_file, standard_data):
    """Calcula el speedup desde el CSV"""
    speedup_data = defaultdict(lambda: defaultdict(list))
    
    script_dir = Path(__file__).parent
    csv_path = script_dir / csv_file
    try:
        with open(csv_path, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                archivo = row['Archivo'].replace('.doc', '')
                
                # Saltar archivo estándar
                if archivo == 'standar':
                    continue
                
                tamaño = row['Tamaño'].split('x')[0]  # Ej: "1000x1000" -> "1000"
                tiempo = float(row['Tiempo (s)'])
                
                # Obtener tiempo base
                if tamaño in standard_data:
                    base_time = standard_data[tamaño]
                    speedup = base_time / tiempo if tiempo > 0 else 0
                    speedup_data[archivo][tamaño].append(speedup)
    except Exception as e:
        print(f"Error leyendo CSV: {e}")
        return {}
    
    # Promediar speedup para cada tamaño
    for archivo in speedup_data:
        for tamaño in speedup_data[archivo]:
            speedup_data[archivo][tamaño] = np.mean(speedup_data[archivo][tamaño])
    
    return speedup_data

def plot_speedup(speedup_data, standard_data):
    """Crea la gráfica de speedup"""
    
    # Orden de tamaños
    sizes_order = sorted(standard_data.keys(), key=int)
    
    # Colores y estilos
    colors = {
        '2hilos': '#1f77b4',
        '4hilos': '#ff7f0e',
        '6hilos': '#2ca02c',
        '8hilos': '#d62728',
        '2procesos': '#9467bd',
        '4procesos': '#8c564b',
        '6procesos': '#e377c2',
        '8procesos': '#7f7f7f',
        'ofast': '#bcbd22',
        'hilos82': '#17becf'
    }
    
    linestyles = {
        '2hilos': '--',
        '4hilos': '--',
        '6hilos': '--',
        '8hilos': '--',
        '2procesos': '-',
        '4procesos': '-',
        '6procesos': '-',
        '8procesos': '-',
        'ofast': '-',
        'hilos82': '-'
    }
    
    markers = {
        '2hilos': 'o',
        '4hilos': 's',
        '6hilos': '^',
        '8hilos': 'D',
        '2procesos': 'o',
        '4procesos': 's',
        '6procesos': '^',
        '8procesos': 'D',
        'ofast': 'X'
    }
    
    # Crear figura
    plt.figure(figsize=(14, 8))
    
    # Plotear cada configuración
    for archivo in sorted(speedup_data.keys()):
        x_values = []
        y_values = []
        
        for size in sizes_order:
            if size in speedup_data[archivo]:
                x_values.append(int(size))
                y_values.append(speedup_data[archivo][size])
        
        if x_values:
            label = f"{archivo}".replace('hilos', ' hilos').replace('procesos', ' procesos')
            plt.plot(x_values, y_values,
                    color=colors.get(archivo, '#000000'),
                    linestyle=linestyles.get(archivo, '-'),
                    marker=markers.get(archivo, 'o'),
                    linewidth=2.5,
                    markersize=8,
                    label=label)
    
    # Configurar gráfica
    plt.xlabel('Tamaño de Matriz', fontsize=12, fontweight='bold')
    plt.ylabel('Speedup', fontsize=12, fontweight='bold')
    plt.title('Speedup - Multiplicación de Matrices', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.legend(loc='upper left', fontsize=10, framealpha=0.95)
    
    # Formatear eje X
    size_ticks = [int(s) for s in sizes_order]
    plt.xticks(size_ticks, size_ticks, rotation=45)
    
    # Añadir línea de referencia (speedup ideal = número de threads/procesos)
    plt.axhline(y=1, color='black', linestyle=':', linewidth=1, alpha=0.5, label='Baseline (1x)')
    
    plt.tight_layout()
    
    # Guardar figura
    script_dir = Path(__file__).parent
    output_file = script_dir / 'speedup_graph.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Gráfica guardada en: {output_file}")

def main():
    print("=" * 70)
    print("ANÁLISIS DE SPEEDUP - MULTIPLICACIÓN DE MATRICES")
    print("=" * 70)
    print()
    
    # Extraer datos estándar
    print("Extrayendo datos de referencia (standar.doc)...")
    standard_data = extract_standard_data()
    
    if not standard_data:
        print("Error: No se pudieron extraer datos de standar.doc")
        return
    
    print(f"✓ Datos de referencia: {len(standard_data)} tamaños")
    print()
    
    # Calcular speedup
    print("Calculando speedup...")
    speedup_data = calculate_speedup('resultados.csv', standard_data)
    
    if not speedup_data:
        print("Error: No se pudieron calcular speedups")
        return
    
    print(f"✓ Speedup calculado para {len(speedup_data)} configuraciones")
    
    # Mostrar tabla de speedups
    print("\n" + "=" * 70)
    print("TABLA DE SPEEDUP")
    print("=" * 70)
    
    sizes_order = sorted(standard_data.keys(), key=int)
    
    print(f"\n{'Configuración':<15}", end='')
    for size in sizes_order:
        print(f"{size:<12}", end='')
    print()
    print("-" * (15 + len(sizes_order) * 12))
    
    for archivo in sorted(speedup_data.keys()):
        print(f"{archivo:<15}", end='')
        for size in sizes_order:
            if size in speedup_data[archivo]:
                speedup = speedup_data[archivo][size]
                print(f"{speedup:<12.2f}", end='')
            else:
                print(f"{'N/A':<12}", end='')
        print()
    
    print()
    
    # Crear gráfica
    print("Creando gráfica...")
    plot_speedup(speedup_data, standard_data)

if __name__ == "__main__":
    main()
