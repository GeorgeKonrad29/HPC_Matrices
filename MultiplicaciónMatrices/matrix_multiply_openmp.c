#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

// Función para crear una matriz
int** crear_matriz(int n) {
    int** matriz = (int**)malloc(n * sizeof(int*));
    for (int i = 0; i < n; i++) {
        matriz[i] = (int*)malloc(n * sizeof(int));
    }
    return matriz;
}

// Función para liberar la memoria de una matriz
void liberar_matriz(int** matriz, int n) {
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

// Función para llenar la matriz con valores aleatorios
void llenar_matriz(int** matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matriz[i][j] = (rand() % 100) + 1;  // Valores entre 1 y 100
        }
    }
}

static double wall_time_seconds(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

// Función para multiplicar dos matrices cuadradas con OpenMP
int** multiplicar_matrices_omp(int** A, int** B, int n) {
    int** C = crear_matriz(n);

    #pragma omp parallel for collapse(3) schedule(static)
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            int suma = 0;
            for (int k = 0; k < n; k++) {
                suma += A[i][k] * B[k][j];
            }
            C[i][j] = suma;
        }
    }

    return C;
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        printf("Uso: %s <tamaño>\n", argv[0]);
        printf("Ejemplo: %s 100\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);

    if (n <= 0) {
        printf("Error: El tamaño debe ser un número positivo.\n");
        n = 2000;
    }

    printf("=== Multiplicación de Matrices Cuadradas con OpenMP ===\n");
    printf("Tamaño de las matrices: %dx%d\n", n, n);
    printf("Hilos OpenMP disponibles: %d\n\n", omp_get_max_threads());

    srand(time(NULL));

    // Crear matrices
    int** A = crear_matriz(n);
    int** B = crear_matriz(n);

    // Llenar matrices con valores aleatorios
    llenar_matriz(A, n);
    llenar_matriz(B, n);

    // Multiplicar matrices
    printf("\nMultiplicando matrices C = A × B...\n");
    double inicio = wall_time_seconds();
    int** C = multiplicar_matrices_omp(A, B, n);
    double fin = wall_time_seconds();
    double tiempo = fin - inicio;

    printf("Tiempo de multiplicación: %.4f segundos\n\n", tiempo);

    if (atoi(argv[1]) <= 10) {
        printf("Matriz A:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                printf("%4d ", A[i][j]);
            }
            printf("\n");
        }

        printf("\nMatriz B:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                printf("%4d ", B[i][j]);
            }
            printf("\n");
        }

        printf("\nMatriz C = A × B:\n");
        for (int i = 0; i < n; i++) {
            for (int j = 0; j < n; j++) {
                printf("%4d ", C[i][j]);
            }
            printf("\n");
        }
    }

    // Liberar memoria
    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);

    printf("¡Programa completado exitosamente!\n");

    return 0;
}