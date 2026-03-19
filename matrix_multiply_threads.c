#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

typedef struct {
    int **A;
    int **B;
    int **C;
    int n;
    int row_start;
    int row_end;
} WorkerArgs;

static double wall_time_seconds(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int **crear_matriz(int n) {
    int **matriz = (int **)malloc(n * sizeof(int *));
    if (!matriz) {
        return NULL;
    }
    for (int i = 0; i < n; i++) {
        matriz[i] = (int *)malloc(n * sizeof(int));
        if (!matriz[i]) {
            for (int j = 0; j < i; j++) {
                free(matriz[j]);
            }
            free(matriz);
            return NULL;
        }
    }
    return matriz;
}

static void liberar_matriz(int **matriz, int n) {
    if (!matriz) {
        return;
    }
    for (int i = 0; i < n; i++) {
        free(matriz[i]);
    }
    free(matriz);
}

static void llenar_matriz(int **matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matriz[i][j] = (rand() % 100) + 1;
        }
    }
}


static void *multiplicar_parcial(void *args_ptr) {
    WorkerArgs *args = (WorkerArgs *)args_ptr;
    for (int i = args->row_start; i < args->row_end; i++) {
        for (int j = 0; j < args->n; j++) {
            int sum = 0;
            for (int k = 0; k < args->n; k++) {
                sum += args->A[i][k] * args->B[k][j];
            }
            args->C[i][j] = sum;
        }
    }
    return NULL;
}

static int multiplicar_matrices_nhilos(int **A, int **B, int **C, int n, int numHilos) {
    pthread_t threads[numHilos];
    WorkerArgs args[numHilos];

    int chunk_size = n / numHilos;
    int remaining_rows = n % numHilos;

    for (int i = 0; i < numHilos; i++) {
        args[i].A = A;
        args[i].B = B;
        args[i].C = C;
        args[i].n = n;
        args[i].row_start = i * chunk_size;
        args[i].row_end = (i + 1) * chunk_size;
        if (i == numHilos - 1) {
            args[i].row_end += remaining_rows;
        }
        if (pthread_create(&threads[i], NULL, multiplicar_parcial, &args[i]) != 0) {
            return -1;
        }
    }
    for (int i = 0; i < numHilos; i++) {
        pthread_join(threads[i], NULL);
    }
    return 0;
}

int main(int argc, char *argv[]) {
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


    printf("Tamaño de las matrices: %dx%d\n\n", n, n);

    srand((unsigned int)time(NULL));

    
    int **A = crear_matriz(n);
    int **B = crear_matriz(n);
    int **C = crear_matriz(n);

    if (!A || !B || !C) {
        printf("Error: no se pudo reservar memoria.\n");
        liberar_matriz(A, n);
        liberar_matriz(B, n);
        liberar_matriz(C, n);
        return 1;
    }


    llenar_matriz(A, n);
    llenar_matriz(B, n);



    printf("\nMultiplicando matrices C = A × B con %d hilos...\n", 8);
    double inicio = wall_time_seconds();
    if (multiplicar_matrices_nhilos(A, B, C, n, 8) != 0) {
        printf("Error: no se pudieron crear los hilos.\n");
        liberar_matriz(A, n);
        liberar_matriz(B, n);
        liberar_matriz(C, n);
        return 1;
    }
    double fin = wall_time_seconds();
    double tiempo = fin - inicio;

    printf("Tiempo de multiplicación: %.4f segundos\n\n", tiempo);



    liberar_matriz(A, n);
    liberar_matriz(B, n);
    liberar_matriz(C, n);



    return 0;
}
