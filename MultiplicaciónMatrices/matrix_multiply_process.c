#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/shm.h>
#include <unistd.h>

static double wall_time_seconds(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0.0;
    }
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1e9;
}

static int **crear_matriz_en_shm(int n, int *shmid) {
    size_t size = n * n * sizeof(int);
    *shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0666);
    
    if (*shmid == -1) {
        return NULL;
    }

    int *ptr = (int *)shmat(*shmid, NULL, 0);
    if (ptr == (int *)-1) {
        shmctl(*shmid, IPC_RMID, NULL);
        return NULL;
    }

    int **matriz = (int **)malloc(n * sizeof(int *));
    for (int i = 0; i < n; i++) {
        matriz[i] = ptr + i * n;
    }

    return matriz;
}

static void liberar_matriz_shm(int **matriz, int n, int shmid) {
    if (!matriz) return;
    shmdt(matriz[0]);
    shmctl(shmid, IPC_RMID, NULL);
    free(matriz);
}

static void llenar_matriz(int **matriz, int n) {
    for (int i = 0; i < n; i++) {
        for (int j = 0; j < n; j++) {
            matriz[i][j] = (rand() % 100) + 1;
        }
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2 || argc > 3) {
        printf("Uso: %s <tamaño> [num_procesos]\n", argv[0]);
        return 1;
    }

    int n = atoi(argv[1]);
    int numProcesos = (argc == 3) ? atoi(argv[2]) : 4;
    
    if (n <= 0 || numProcesos <= 0) {
        printf("Error: tamaño y procesos deben ser positivos.\n");
        return 1;
    }

    srand((unsigned int)time(NULL));

    // Crear matrices en memoria compartida
    int shmid_A, shmid_B, shmid_C;
    int **A = crear_matriz_en_shm(n, &shmid_A);
    int **B = crear_matriz_en_shm(n, &shmid_B);
    int **C = crear_matriz_en_shm(n, &shmid_C);

    if (!A || !B || !C) {
        printf("Error: no se pudo crear memoria compartida.\n");
        return 1;
    }

    llenar_matriz(A, n);
    llenar_matriz(B, n);

    printf("Tamaño: %dx%d\n", n, n);
    fflush(stdout);

    double inicio = wall_time_seconds();

    int chunk_size = n / numProcesos;
    int remaining_rows = n % numProcesos;

    pid_t pids[numProcesos];
    
    for (int i = 0; i < numProcesos; i++) {
        pid_t pid = fork();
        if (pid == 0) {
            // Proceso hijo
            int row_start = i * chunk_size;
            int row_end = (i + 1) * chunk_size;
            if (i == numProcesos - 1) {
                row_end += remaining_rows;
            }

            // Acceder a la memoria compartida
            int *ptr_A = (int *)shmat(shmid_A, NULL, 0);
            int *ptr_B = (int *)shmat(shmid_B, NULL, 0);
            int *ptr_C = (int *)shmat(shmid_C, NULL, 0);

            int **A_local = (int **)malloc(n * sizeof(int *));
            int **B_local = (int **)malloc(n * sizeof(int *));
            int **C_local = (int **)malloc(n * sizeof(int *));

            for (int j = 0; j < n; j++) {
                A_local[j] = ptr_A + j * n;
                B_local[j] = ptr_B + j * n;
                C_local[j] = ptr_C + j * n;
            }

            // Calcular su parte
            for (int row = row_start; row < row_end; row++) {
                for (int j = 0; j < n; j++) {
                    int sum = 0;
                    for (int k = 0; k < n; k++) {
                        sum += A_local[row][k] * B_local[k][j];
                    }
                    C_local[row][j] = sum;
                }
            }

            free(A_local);
            free(B_local);
            free(C_local);
            
            exit(0);
        } else if (pid > 0) {
            pids[i] = pid;
        } else {
            printf("Error: no se pudo crear el proceso.\n");
            return 1;
        }
    }

    // Esperar a que terminen todos los procesos
    for (int i = 0; i < numProcesos; i++) {
        waitpid(pids[i], NULL, 0);
    }

    double fin = wall_time_seconds();
    printf("Tiempo: %.4f segundos\n", fin - inicio);

    liberar_matriz_shm(A, n, shmid_A);
    liberar_matriz_shm(B, n, shmid_B);
    liberar_matriz_shm(C, n, shmid_C);

    return 0;
}
