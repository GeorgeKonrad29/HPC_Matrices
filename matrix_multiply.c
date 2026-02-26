#include <stdio.h>
#include <stdlib.h>
#include <time.h>

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

// Función para multiplicar dos matrices cuadradas
int** multiplicar_matrices(int** A, int** B, int n) {
    int** C = crear_matriz(n);
    
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            C[i][j] = 0;
            for (int k = 0; k < n; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
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
        return 1;
    }
    
    printf("=== Multiplicación de Matrices Cuadradas ===\n");
    printf("Tamaño de las matrices: %dx%d\n\n", n, n);
    
    srand(time(NULL));
    
    // Crear matrices
    printf("Creando matrices...\n");
    int** A = crear_matriz(n);
    int** B = crear_matriz(n);
    
    // Llenar matrices con valores aleatorios
    printf("Llenando matrices con valores aleatorios...\n");
    llenar_matriz(A, n);
    llenar_matriz(B, n);
    

    // Multiplicar matrices
    printf("\nMultiplicando matrices C = A × B...\n");
    double inicio = wall_time_seconds();
    int** C = multiplicar_matrices(A, B, n);
    double fin = wall_time_seconds();
    double tiempo = fin - inicio;
    
    printf("Tiempo de multiplicación: %.4f segundos\n\n", tiempo);
    

    
    // Liberar memoria
    printf("\nLiberando memoria...\n");
    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);
    
    printf("¡Programa completado exitosamente!\n");
    
    return 0;
}
