# Reto 2: Simulaci\u00f3n de tr\u00e1fico en una carretera peri\u00f3dica

Este proyecto implementa una simulaci\u00f3n de autos sobre una carretera de $N$ celdas con condiciones peri\u00f3dicas. Cada celda puede estar vac\u00eda (`0`) o contener un auto (`1`). En cada iteraci\u00f3n, un auto avanza una celda si el espacio de adelante est\u00e1 libre; de lo contrario, permanece en su sitio.

El programa calcula adem\u00e1s la velocidad media en cada paso, definida como:

$$
v_{med} = \frac{\text{autos que se movieron en la iteraci\u00f3n}}{\text{n\u00famero total de autos}}
$$

## Versi\u00f3n serial

El archivo [reto2.c](reto2.c) contiene la versi\u00f3n secuencial inicial del problema. Esta versi\u00f3n usa dos arreglos separados:

- uno para el estado actual de la carretera
- otro para construir el siguiente estado

Ese enfoque garantiza que la actualizaci\u00f3n dependa solo del estado anterior, lo que facilita una futura paralelizaci\u00f3n con OpenMP.

## Compilaci\u00f3n

```bash
gcc -std=c11 -Wall -Wextra -pedantic reto2.c -o reto2
```

## Ejecuci\u00f3n

```bash
./reto2 [cells] [steps] [density] [seed]
```

Par\u00e1metros:

- `cells`: n\u00famero de celdas de la carretera. Valor por defecto: `100`
- `steps`: n\u00famero de iteraciones de la simulaci\u00f3n. Valor por defecto: `50`
- `density`: densidad inicial de autos en el rango `[0, 1]`. Valor por defecto: `0.30`
- `seed`: semilla para la inicializaci\u00f3n determinista. Valor por defecto: `1`

Ejemplo:

```bash
./reto2 50 20 0.35 7
```

## Salida

El programa muestra:

- los par\u00e1metros usados
- el n\u00famero de autos iniciales
- por cada paso, cu\u00e1ntos autos se movieron y la velocidad media
- el estado final de la carretera
- el tiempo total de ejecuci\u00f3n

## Nota para la versi\u00f3n OpenMP

La estructura actual est\u00e1 pensada para extenderse a una versi\u00f3n paralela con OpenMP sin cambiar la l\u00f3gica del modelo. El siguiente paso natural es paralelizar el ciclo que construye el estado siguiente de la carretera.
