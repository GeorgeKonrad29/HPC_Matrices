#!/usr/bin/env python3

import csv
import matplotlib.pyplot as plt
import numpy as np
from collections import defaultdict
from pathlib import Path

def extract_execution_times(csv_file):
    """Extrae tiempos de ejecución del CSV"""
    time_data = defaultdict(lambda: defaultdict(list))
    
    try:
        with open(csv_file, 'r') as f:
            reader = csv.DictReader(f)
            for row in reader:
                archivo = row['Archivo'].replace('.doc', '')
                tamaño = row['Tamaño'].split('x')[0]  # Ej: "1000x1000" -> "1000"
                tiempo = float(row['Tiempo (s)'])
                
                time_data[archivo][tamaño].append(tiempo)
    except Exception as e:
        print(f"Error leyendo CSV: {e}")
        return {}
    
    # Promediar tiempos para cada tamaño
    for archivo in time_data:
        for tamaño in time_data[archivo]:
            time_data[archivo][tamaño] = np.mean(time_data[archivo][tamaño])
    
    return time_data

def plot_execution_times(time_data):
    """Crea la gráfica de tiempo de ejecución"""
    
    # Orden de tamaños
    all_sizes = set()
    for archivo in time_data:
        all_sizes.update(time_data[archivo].keys())
    sizes_order = sorted(all_sizes, key=int)
    
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
        'standar': '#000000',
        'cache': '#17becf',
        'ofast': '#bcbd22'
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
        'standar': '-',
        'cache': '-',
        'ofast': '-'
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
        'standar': 'x',
        'cache': '+',
        'ofast': '*'
    }
    
    # Crear figura
    plt.figure(figsize=(14, 8))
    
    # Plotear cada configuración
    for archivo in sorted(time_data.keys()):
        x_values = []
        y_values = []
        
        for size in sizes_order:
            if size in time_data[archivo]:
                x_values.append(int(size))
                y_values.append(time_data[archivo][size])
        
        if x_values:
            # Formatear etiqueta
            label = archivo.replace('hilos', ' hilos').replace('procesos', ' procesos')
            
            plt.plot(x_values, y_values,
                    color=colors.get(archivo, '#000000'),
                    linestyle=linestyles.get(archivo, '-'),
                    marker=markers.get(archivo, 'o'),
                    linewidth=2.5,
                    markersize=8,
                    label=label)
    
    # Configurar gráfica
    plt.xlabel('Tamaño de Matriz', fontsize=12, fontweight='bold')
    plt.ylabel('Tiempo de Ejecución (segundos)', fontsize=12, fontweight='bold')
    plt.title('Tiempo de Ejecución vs Tamaño de Matriz', fontsize=14, fontweight='bold')
    plt.grid(True, alpha=0.3, linestyle='--')
    plt.legend(loc='upper left', fontsize=10, framealpha=0.95)
    
    # Formatear eje X
    size_ticks = [int(s) for s in sizes_order]
    plt.xticks(size_ticks, size_ticks, rotation=45)
    
    plt.tight_layout()
    
    # Guardar figura
    output_file = 'execution_time_graph.png'
    plt.savefig(output_file, dpi=300, bbox_inches='tight')
    print(f"✓ Gráfica guardada en: {output_file}")

def print_table(time_data):
    """Imprime tabla de tiempos promedio"""
    all_sizes = set()
    for archivo in time_data:
        all_sizes.update(time_data[archivo].keys())
    sizes_order = sorted(all_sizes, key=int)
    
    print("\n" + "=" * 70)
    print("TABLA DE TIEMPOS DE EJECUCIÓN (segundos)")
    print("=" * 70)
    
    print(f"\n{'Configuración':<15}", end='')
    for size in sizes_order:
        print(f"{size:<12}", end='')
    print()
    print("-" * (15 + len(sizes_order) * 12))
    
    for archivo in sorted(time_data.keys()):
        print(f"{archivo:<15}", end='')
        for size in sizes_order:
            if size in time_data[archivo]:
                tiempo = time_data[archivo][size]
                print(f"{tiempo:<12.2f}", end='')
            else:
                print(f"{'N/A':<12}", end='')
        print()

def main():
    print("=" * 70)
    print("ANÁLISIS DE TIEMPO DE EJECUCIÓN - MULTIPLICACIÓN DE MATRICES")
    print("=" * 70)
    print()
    
    # Extraer tiempos de ejecución
    print("Extrayendo tiempos de ejecución...")
    time_data = extract_execution_times('resultados.csv')
    
    if not time_data:
        print("Error: No se pudieron extraer tiempos de ejecución")
        return
    
    print(f"✓ Tiempos extraídos para {len(time_data)} configuraciones")
    print()
    
    # Mostrar tabla
    print_table(time_data)
    
    # Crear gráfica
    print("\nCreando gráfica...")
    plot_execution_times(time_data)

if __name__ == "__main__":
    main()
