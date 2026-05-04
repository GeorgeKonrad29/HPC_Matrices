#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

static void printUsage(const char *programName) {
    fprintf(stderr,
            "Usage: %s [cells] [steps] [density] [seed]\n"
            "  cells   : number of road cells (default: 100)\n"
            "  steps   : number of simulation steps (default: 50)\n"
            "  density : initial car density in [0,1] (default: 0.30)\n"
            "  seed    : seed for deterministic initialization (default: 1)\n",
            programName);
}

static int parseSizeT(const char *text, size_t *value) {
    char *end = NULL;
    errno = 0;
    unsigned long long parsed = strtoull(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    *value = (size_t)parsed;
    return 1;
}

static int parseUint32(const char *text, uint32_t *value) {
    char *end = NULL;
    errno = 0;
    unsigned long parsed = strtoul(text, &end, 10);
    if (errno != 0 || end == text || *end != '\0' || parsed > UINT32_MAX) {
        return 0;
    }
    *value = (uint32_t)parsed;
    return 1;
}

static int parseDouble(const char *text, double *value) {
    char *end = NULL;
    errno = 0;
    double parsed = strtod(text, &end);
    if (errno != 0 || end == text || *end != '\0') {
        return 0;
    }
    *value = parsed;
    return 1;
}

static uint32_t rngNext(uint32_t *state) {
    *state = (*state * 1664525u) + 1013904223u;
    return *state;
}

static void shuffleIndices(size_t *indices, size_t count, uint32_t seed) {
    uint32_t state = seed == 0 ? 1u : seed;

    if (count < 2) {
        return;
    }

    for (size_t i = count - 1; i > 0; --i) {
        size_t j = (size_t)(rngNext(&state) % (uint32_t)(i + 1));
        size_t tmp = indices[i];
        indices[i] = indices[j];
        indices[j] = tmp;
    }
}

static void buildCountsAndDispls(size_t totalCells, int size, int *counts, int *displs) {
    size_t base = totalCells / (size_t)size;
    size_t remainder = totalCells % (size_t)size;

    int offset = 0;
    for (int rank = 0; rank < size; ++rank) {
        size_t local = base + ((size_t)rank < remainder ? 1u : 0u);
        counts[rank] = (int)local;
        displs[rank] = offset;
        offset += counts[rank];
    }
}

static void initializeRoad(unsigned char *road, size_t cellCount, double density, uint32_t seed) {
    size_t *indices = (size_t *)malloc(cellCount * sizeof(size_t));
    if (indices == NULL) {
        return;
    }

    for (size_t i = 0; i < cellCount; ++i) {
        indices[i] = i;
    }

    shuffleIndices(indices, cellCount, seed);

    size_t carCount = (size_t)((density * (double)cellCount) + 0.5);
    if (carCount > cellCount) {
        carCount = cellCount;
    }

    memset(road, 0, cellCount * sizeof(unsigned char));
    for (size_t i = 0; i < carCount; ++i) {
        road[indices[i]] = 1u;
    }

    free(indices);
}

static size_t updateLocalRoad(const unsigned char *localRoad,
                              unsigned char *nextRoad,
                              size_t localCount) {
    size_t moved = 0;

    // localRoad[0] es la celda fantasma izquierda (del rank-1)
    // localRoad[localCount+1] es la celda fantasma derecha (del rank+1)

    for (size_t j = 1; j <= localCount; ++j) {
        unsigned char self  = localRoad[j];
        unsigned char left  = localRoad[j - 1];
        unsigned char right = localRoad[j + 1];

        // REGLA 1: Un coche entra desde la izquierda
        if (left == 1u && self == 0u) {
            nextRoad[j] = 1u;
            ++moved;
        } 
        // REGLA 2: Tengo un coche y NO puede moverse porque el de la derecha le bloquea
        else if (self == 1u && right == 1u) {
            nextRoad[j] = 1u;
        }
        // REGLA 3: Mi celda queda vacía (ya estaba vacía o mi coche se movió a la derecha)
        else {
            nextRoad[j] = 0u;
        }
    }

    return moved;
}

static void printRoad(const unsigned char *road, size_t cellCount) {
    for (size_t i = 0; i < cellCount; ++i) {
        putchar(road[i] ? '1' : '0');
    }
    putchar('\n');
}

int main(int argc, char *argv[]) {
    int rank = 0;
    int size = 1;

    size_t totalCells = 100;
    size_t stepCount = 50;
    double initialDensity = 0.30;
    uint32_t rngSeed = 1u;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
        if (rank == 0) {
            printUsage(argv[0]);
        }
        MPI_Finalize();
        return EXIT_SUCCESS;
    }

    if (argc > 1 && !parseSizeT(argv[1], &totalCells)) {
        if (rank == 0) {
            fprintf(stderr, "Error: invalid cells value.\n");
            printUsage(argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (argc > 2 && !parseSizeT(argv[2], &stepCount)) {
        if (rank == 0) {
            fprintf(stderr, "Error: invalid steps value.\n");
            printUsage(argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (argc > 3 && !parseDouble(argv[3], &initialDensity)) {
        if (rank == 0) {
            fprintf(stderr, "Error: invalid density value.\n");
            printUsage(argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (argc > 4 && !parseUint32(argv[4], &rngSeed)) {
        if (rank == 0) {
            fprintf(stderr, "Error: invalid seed value.\n");
            printUsage(argv[0]);
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (totalCells == 0 || stepCount == 0 || initialDensity < 0.0 || initialDensity > 1.0) {
        if (rank == 0) {
            if (totalCells == 0) {
                fprintf(stderr, "Error: cells must be greater than zero.\n");
            } else if (stepCount == 0) {
                fprintf(stderr, "Error: steps must be greater than zero.\n");
            } else {
                fprintf(stderr, "Error: density must be in the range [0, 1].\n");
            }
        }
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    int *counts = (int *)calloc((size_t)size, sizeof(int));
    int *displs = (int *)calloc((size_t)size, sizeof(int));
    if (counts == NULL || displs == NULL) {
        if (rank == 0) {
            fprintf(stderr, "Error: unable to allocate counts/displs.\n");
        }
        free(counts);
        free(displs);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    buildCountsAndDispls(totalCells, size, counts, displs);

    int localCount = counts[rank];
    unsigned char *localRoad = (unsigned char *)calloc((size_t)localCount + 2u, sizeof(unsigned char));
    unsigned char *nextRoad = (unsigned char *)calloc((size_t)localCount + 2u, sizeof(unsigned char));
    unsigned char *globalRoad = NULL;
    unsigned char *nextGlobalRoad = NULL;

    if (localRoad == NULL || nextRoad == NULL) {
        if (rank == 0) {
            fprintf(stderr, "Error: unable to allocate local buffers.\n");
        }
        free(localRoad);
        free(nextRoad);
        free(counts);
        free(displs);
        MPI_Finalize();
        return EXIT_FAILURE;
    }

    if (rank == 0) {
        globalRoad = (unsigned char *)calloc(totalCells, sizeof(unsigned char));
        nextGlobalRoad = (unsigned char *)calloc(totalCells, sizeof(unsigned char));
        if (globalRoad == NULL || nextGlobalRoad == NULL) {
            fprintf(stderr, "Error: unable to allocate global buffers.\n");
            free(globalRoad);
            free(nextGlobalRoad);
            free(localRoad);
            free(nextRoad);
            free(counts);
            free(displs);
            MPI_Finalize();
            return EXIT_FAILURE;
        }

        initializeRoad(globalRoad, totalCells, initialDensity, rngSeed);
    }

    MPI_Scatterv(globalRoad, counts, displs, MPI_UNSIGNED_CHAR,
                 &localRoad[1], localCount, MPI_UNSIGNED_CHAR,
                 0, MPI_COMM_WORLD);
    double startTime, endTime;

    
    MPI_Barrier(MPI_COMM_WORLD);
    startTime = MPI_Wtime();

    for (size_t step = 1; step <= stepCount; ++step) {
        int leftNeighbor = (rank == 0) ? size - 1 : rank - 1;
        int rightNeighbor = (rank == size - 1) ? 0 : rank + 1;

        MPI_Sendrecv(&localRoad[1], 1, MPI_UNSIGNED_CHAR, leftNeighbor, 0,
                     &localRoad[(size_t)localCount + 1u], 1, MPI_UNSIGNED_CHAR, rightNeighbor, 0,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        MPI_Sendrecv(&localRoad[localCount], 1, MPI_UNSIGNED_CHAR, rightNeighbor, 1,
                     &localRoad[0], 1, MPI_UNSIGNED_CHAR, leftNeighbor, 1,
                     MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        size_t localMoved = updateLocalRoad(localRoad, nextRoad, (size_t)localCount);
        size_t totalMoved = 0;

        MPI_Reduce(&localMoved, &totalMoved, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

        /*if (rank == 0) {
            printf("Step %zu: %zu cars moved\n", step, totalMoved);
            printRoad(globalRoad, totalCells); // Si quieres imprimir el estado global (requiere gather)             
        }*/

        unsigned char *tmp = localRoad;
        localRoad = nextRoad;
        nextRoad = tmp;
    }

    MPI_Gatherv(&localRoad[1], localCount, MPI_UNSIGNED_CHAR,
                nextGlobalRoad, counts, displs, MPI_UNSIGNED_CHAR,
                0, MPI_COMM_WORLD);
    
    MPI_Barrier(MPI_COMM_WORLD);
    endTime = MPI_Wtime();
    // ... (Después de MPI_Barrier y el cálculo de endTime - startTime) ...

    if (rank == 0) {
        double wallTime = endTime - startTime;
        
        
        // fprintf(stderr, "Simulación completada en %f s\n", wallTime);

        // Formato para SCRIPTS (Celdas, Pasos, Densidad, Procesos, Tiempo)
        // Usamos printf para que sea la salida estándar que capture el script
        printf("%zu,%zu,%.2f,%d,%.6f\n", 
                totalCells, stepCount, initialDensity, size, wallTime);
    }
    free(globalRoad);
    free(nextGlobalRoad);
    free(localRoad);
    free(nextRoad);
    free(counts);
    free(displs);
    MPI_Finalize();
    return EXIT_SUCCESS;
}