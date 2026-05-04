#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <time.h>

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

static int initializeRoad(unsigned char *road, size_t cellCount, double density, uint32_t seed, size_t *carsOut) {
	size_t *indices = (size_t *)malloc(cellCount * sizeof(size_t));
	if (indices == NULL) {
		return 0;
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
	*carsOut = carCount;
	return 1;
}

static size_t advanceRoad(const unsigned char *currentRoad, unsigned char *nextRoad, size_t cellCount) {
	memset(nextRoad, 0, cellCount * sizeof(unsigned char));

	size_t moved = 0;
	for (size_t i = 0; i < cellCount; ++i) {
		if (!currentRoad[i]) {
			continue;
		}

		size_t frontCell = (i + 1 == cellCount) ? 0 : (i + 1);
		if (!currentRoad[frontCell]) {
			nextRoad[frontCell] = 1u;
			moved++;
		} else {
			nextRoad[i] = 1u;
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
	size_t cellCount = 100;
	size_t stepCount = 50;
	double initialDensity = 0.30;
	uint32_t rngSeed = 1u;

	if (argc > 1 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)) {
		printUsage(argv[0]);
		return EXIT_SUCCESS;
	}

	if (argc > 1 && !parseSizeT(argv[1], &cellCount)) {
		fprintf(stderr, "Error: invalid cells value.\n");
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	if (argc > 2 && !parseSizeT(argv[2], &stepCount)) {
		fprintf(stderr, "Error: invalid steps value.\n");
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	if (argc > 3 && !parseDouble(argv[3], &initialDensity)) {
		fprintf(stderr, "Error: invalid density value.\n");
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	if (argc > 4 && !parseUint32(argv[4], &rngSeed)) {
		fprintf(stderr, "Error: invalid seed value.\n");
		printUsage(argv[0]);
		return EXIT_FAILURE;
	}

	if (cellCount == 0) {
		fprintf(stderr, "Error: cells must be greater than zero.\n");
		return EXIT_FAILURE;
	}

	if (stepCount == 0) {
		fprintf(stderr, "Error: steps must be greater than zero.\n");
		return EXIT_FAILURE;
	}

	if (initialDensity < 0.0 || initialDensity > 1.0) {
		fprintf(stderr, "Error: density must be in the range [0, 1].\n");
		return EXIT_FAILURE;
	}

	unsigned char *currentRoad = (unsigned char *)calloc(cellCount, sizeof(unsigned char));
	unsigned char *nextRoad = (unsigned char *)calloc(cellCount, sizeof(unsigned char));
	if (currentRoad == NULL || nextRoad == NULL) {
		fprintf(stderr, "Error: unable to allocate memory for the road.\n");
		free(currentRoad);
		free(nextRoad);
		return EXIT_FAILURE;
	}

	size_t totalCars = 0;
	if (!initializeRoad(currentRoad, cellCount, initialDensity, rngSeed, &totalCars)) {
		fprintf(stderr, "Error: unable to initialize the road.\n");
		free(currentRoad);
		free(nextRoad);
		return EXIT_FAILURE;
	}

	struct timespec startTime;
	struct timespec endTime;
	if (clock_gettime(CLOCK_MONOTONIC, &startTime) != 0) {
		fprintf(stderr, "Error: unable to read CLOCK_MONOTONIC.\n");
		free(currentRoad);
		free(nextRoad);
		return EXIT_FAILURE;
	}

	/*printf("Serial traffic simulation on a periodic road\n");
	printf("Cells: %zu\n", cellCount);
	printf("Steps: %zu\n", stepCount);
	printf("Target density: %.4f\n", initialDensity);
	printf("Seed: %u\n", rngSeed);
	printf("Initial cars: %zu\n", totalCars);
	printf("\n");
	printf("Step   Moved   AvgVelocity\n");
	printf("----   -----   -----------\n");*/

	for (size_t step = 1; step <= stepCount; ++step) {
		size_t moved = advanceRoad(currentRoad, nextRoad, cellCount);
		//double averageVelocity = (totalCars > 0) ? ((double)moved / (double)totalCars) : 0.0;

		//printf("%4zu   %5zu   %11.6f\n", step, moved, averageVelocity);

		unsigned char *swapRoad = currentRoad;
		currentRoad = nextRoad;
		nextRoad = swapRoad;
	}

	if (clock_gettime(CLOCK_MONOTONIC, &endTime) != 0) {
		fprintf(stderr, "Error: unable to read CLOCK_MONOTONIC.\n");
		free(currentRoad);
		free(nextRoad);
		return EXIT_FAILURE;
	}

	double elapsedSeconds = (double)(endTime.tv_sec - startTime.tv_sec)
		+ (double)(endTime.tv_nsec - startTime.tv_nsec) * 1.0e-9;

	printf("\n");
	printf("Final road state:\n");
	printRoad(currentRoad, cellCount);
	printf("Wall time: %.6f seconds\n", elapsedSeconds);
    printf("%zu,%zu,%.2f,%d,%.6f\n", 
            cellCount, stepCount, initialDensity, 1, elapsedSeconds);


	free(currentRoad);
	free(nextRoad);
	return EXIT_SUCCESS;
}
