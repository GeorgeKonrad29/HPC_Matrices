#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <pthread.h>

#define MAX_IT 1000000

double exact(double x) {
    return x * (x - 1.0) * exp(x);
}

double force(double x) {
    return -x * (x + 3.0) * exp(x);
}

static double rms_norm_diff(const double *a, const double *b, int n) {
    double sum = 0.0;
    for (int i = 0; i < n; i++) {
        double d = a[i] - b[i];
        sum += d * d;
    }
    return sqrt(sum / (double)n);
}

static void compute_chunk_bounds(int worker_id, int workers, int nk, int *start, int *end) {
    int interior = nk - 2;
    if (interior <= 0 || worker_id >= workers) {
        *start = 1;
        *end = 0;
        return;
    }

    int base = interior / workers;
    int rem = interior % workers;
    int count = base + (worker_id < rem ? 1 : 0);
    int offset = worker_id * base + (worker_id < rem ? worker_id : rem);

    *start = 1 + offset;
    *end = *start + count - 1;
}

static void solve_direct_tridiagonal(int nk, double h2, double ua, double ub, const double *fk, double *u_direct) {
    if (nk <= 0) {
        return;
    }

    if (nk == 1) {
        u_direct[0] = ua;
        return;
    }

    if (nk == 2) {
        u_direct[0] = ua;
        u_direct[1] = ub;
        return;
    }

    int m = nk - 2;
    double *cprime = (double *)malloc((size_t)m * sizeof(double));
    double *dprime = (double *)malloc((size_t)m * sizeof(double));

    if (!cprime || !dprime) {
        fprintf(stderr, "Error: no se pudo asignar memoria para la solución directa.\n");
        free(cprime);
        free(dprime);
        exit(EXIT_FAILURE);
    }

    double a_sub = -1.0 / h2;
    double b_diag = 2.0 / h2;
    double c_sup = -1.0 / h2;

    for (int j = 0; j < m; j++) {
        int i = j + 1;
        double rhs = fk[i];
        if (j == 0) {
            rhs -= a_sub * ua;
        }
        if (j == m - 1) {
            rhs -= c_sup * ub;
        }

        if (j == 0) {
            cprime[j] = c_sup / b_diag;
            dprime[j] = rhs / b_diag;
        } else {
            double denom = b_diag - a_sub * cprime[j - 1];
            cprime[j] = (j == m - 1) ? 0.0 : (c_sup / denom);
            dprime[j] = (rhs - a_sub * dprime[j - 1]) / denom;
        }
    }

    u_direct[0] = ua;
    u_direct[nk - 1] = ub;
    u_direct[nk - 2] = dprime[m - 1];

    for (int j = m - 2; j >= 0; j--) {
        u_direct[j + 1] = dprime[j] - cprime[j] * u_direct[j + 2];
    }

    free(cprime);
    free(dprime);
}

typedef struct {
    int tid;
    int nthreads;
    int nk;
    double h2;
    const double *f;
    double *u_old;
    double *u_new;
    double *local_res;
    double *local_change;
    double tol;
    int *it_num;
    int *stop_flag;
    double *last_residual;
    pthread_barrier_t *barrier;
} ThreadContext;

static void *jacobi_worker(void *arg) {
    ThreadContext *ctx = (ThreadContext *)arg;
    int start, end;
    compute_chunk_bounds(ctx->tid, ctx->nthreads, ctx->nk, &start, &end);

    while (1) {
        for (int i = start; i <= end; i++) {
            ctx->u_new[i] = 0.5 * (ctx->u_old[i - 1] + ctx->u_old[i + 1] + ctx->h2 * ctx->f[i]);
        }

        pthread_barrier_wait(ctx->barrier);

        double sum_sq_res = 0.0;
        double sum_sq_change = 0.0;
        for (int i = start; i <= end; i++) {
            double ri = (-ctx->u_new[i - 1] + 2.0 * ctx->u_new[i] - ctx->u_new[i + 1]) / ctx->h2 - ctx->f[i];
            double du = ctx->u_new[i] - ctx->u_old[i];
            sum_sq_res += ri * ri;
            sum_sq_change += du * du;
        }

        ctx->local_res[ctx->tid] = sum_sq_res;
        ctx->local_change[ctx->tid] = sum_sq_change;

        pthread_barrier_wait(ctx->barrier);

        if (ctx->tid == 0) {
            double total_res = 0.0;
            for (int t = 0; t < ctx->nthreads; t++) {
                total_res += ctx->local_res[t];
            }

            (*ctx->it_num)++;
            *ctx->last_residual = sqrt(total_res / (double)ctx->nk);

            if (*ctx->last_residual <= ctx->tol || *ctx->it_num >= MAX_IT) {
                *ctx->stop_flag = 1;
                if (*ctx->it_num >= MAX_IT && *ctx->last_residual > ctx->tol) {
                    fprintf(stderr, "Advertencia: Jacobi alcanzó el máximo de iteraciones.\n");
                }
            }
        }

        pthread_barrier_wait(ctx->barrier);

        if (*ctx->stop_flag) {
            break;
        }

        for (int i = start; i <= end; i++) {
            ctx->u_old[i] = ctx->u_new[i];
        }

        pthread_barrier_wait(ctx->barrier);
    }

    return NULL;
}

static int jacobi_poisson_1d_threads(int nk, double h2, const double *f, double *u, double tol, int nthreads) {
    double *u_old = (double *)calloc((size_t)nk, sizeof(double));
    double *u_new = (double *)calloc((size_t)nk, sizeof(double));
    double *local_res = (double *)calloc((size_t)nthreads, sizeof(double));
    double *local_change = (double *)calloc((size_t)nthreads, sizeof(double));

    if (!u_old || !u_new || !local_res || !local_change) {
        fprintf(stderr, "Error: no se pudo asignar memoria para Jacobi con hilos.\n");
        free(u_old);
        free(u_new);
        free(local_res);
        free(local_change);
        exit(EXIT_FAILURE);
    }

    u_old[0] = u[0];
    u_old[nk - 1] = u[nk - 1];
    u_new[0] = u[0];
    u_new[nk - 1] = u[nk - 1];

    pthread_t *threads = (pthread_t *)malloc((size_t)nthreads * sizeof(pthread_t));
    ThreadContext *ctxs = (ThreadContext *)malloc((size_t)nthreads * sizeof(ThreadContext));
    pthread_barrier_t barrier;

    if (!threads || !ctxs) {
        fprintf(stderr, "Error: no se pudo asignar memoria para hilos.\n");
        free(u_old);
        free(u_new);
        free(local_res);
        free(local_change);
        free(threads);
        free(ctxs);
        exit(EXIT_FAILURE);
    }

    pthread_barrier_init(&barrier, NULL, (unsigned)nthreads);

    int it_num = 0;
    int stop_flag = 0;
    double last_residual = 0.0;

    for (int t = 0; t < nthreads; t++) {
        ctxs[t].tid = t;
        ctxs[t].nthreads = nthreads;
        ctxs[t].nk = nk;
        ctxs[t].h2 = h2;
        ctxs[t].f = f;
        ctxs[t].u_old = u_old;
        ctxs[t].u_new = u_new;
        ctxs[t].local_res = local_res;
        ctxs[t].local_change = local_change;
        ctxs[t].tol = tol;
        ctxs[t].it_num = &it_num;
        ctxs[t].stop_flag = &stop_flag;
        ctxs[t].last_residual = &last_residual;
        ctxs[t].barrier = &barrier;

        if (pthread_create(&threads[t], NULL, jacobi_worker, &ctxs[t]) != 0) {
            fprintf(stderr, "Error: no se pudo crear el hilo %d.\n", t);
            exit(EXIT_FAILURE);
        }
    }

    for (int t = 0; t < nthreads; t++) {
        pthread_join(threads[t], NULL);
    }

    for (int i = 0; i < nk; i++) {
        u[i] = u_new[i];
    }

    pthread_barrier_destroy(&barrier);
    free(threads);
    free(ctxs);
    free(u_old);
    free(u_new);
    free(local_res);
    free(local_change);

    return it_num;
}

int main(int argc, char *argv[]) {
    int k = 5;
    int nthreads = 4;

    if (argc > 1) {
        k = atoi(argv[1]);
    }
    if (argc > 2) {
        nthreads = atoi(argv[2]);
    }

    if (k < 1) {
        fprintf(stderr, "Error: K debe ser >= 1.\n");
        return EXIT_FAILURE;
    }
    if (nthreads < 1) {
        fprintf(stderr, "Error: número de hilos debe ser >= 1.\n");
        return EXIT_FAILURE;
    }

    int nk = (1 << k) + 1;
    int max_threads = nk - 2;
    if (max_threads < 1) {
        max_threads = 1;
    }
    if (nthreads > max_threads) {
        nthreads = max_threads;
    }

    struct timespec ts_start, ts_end;
    if (clock_gettime(CLOCK_MONOTONIC, &ts_start) != 0) {
        fprintf(stderr, "Error: no se pudo leer CLOCK_MONOTONIC.\n");
        return EXIT_FAILURE;
    }

    double a = 0.0;
    double b = 1.0;
    double ua = 0.0;
    double ub = 0.0;
    double tol = 1.0e-6;
    double hk = (b - a) / (double)(nk - 1);
    double h2 = hk * hk;

    double *xk = (double *)malloc((size_t)nk * sizeof(double));
    double *fk = (double *)malloc((size_t)nk * sizeof(double));
    double *uek = (double *)malloc((size_t)nk * sizeof(double));
    double *udk = (double *)malloc((size_t)nk * sizeof(double));
    double *ujk = (double *)calloc((size_t)nk, sizeof(double));

    if (!xk || !fk || !uek || !udk || !ujk) {
        fprintf(stderr, "Error: no se pudo asignar memoria.\n");
        free(xk);
        free(fk);
        free(uek);
        free(udk);
        free(ujk);
        return EXIT_FAILURE;
    }

    for (int i = 0; i < nk; i++) {
        xk[i] = a + (double)i * hk;
        fk[i] = force(xk[i]);
        uek[i] = exact(xk[i]);
    }
    fk[0] = ua;
    fk[nk - 1] = ub;

    printf("\n");
    printf("JACOBI_POISSON_1D_THREADS:\n");
    printf("  Use Jacobi iteration (pthread) for the 1D Poisson equation.\n");

    solve_direct_tridiagonal(nk, h2, ua, ub, fk, udk);

    ujk[0] = ua;
    ujk[nk - 1] = ub;
    int it_num = jacobi_poisson_1d_threads(nk, h2, fk, ujk, tol, nthreads);

    if (clock_gettime(CLOCK_MONOTONIC, &ts_end) != 0) {
        fprintf(stderr, "Error: no se pudo leer CLOCK_MONOTONIC.\n");
        free(xk);
        free(fk);
        free(uek);
        free(udk);
        free(ujk);
        return EXIT_FAILURE;
    }
    double elapsed_time = (double)(ts_end.tv_sec - ts_start.tv_sec)
        + (double)(ts_end.tv_nsec - ts_start.tv_nsec) * 1.0e-9;

    printf("\n");
    printf("  Using grid index K = %d\n", k);
    printf("  System size NK was %d\n", nk);
    printf("  Threads used = %d\n", nthreads);
    printf("  Tolerance for linear residual %g\n", tol);
    printf("  Number of Jacobi iterations required was %d\n", it_num);
    printf("  RMS Jacobi error in solution of linear system = %g\n", rms_norm_diff(udk, ujk, nk));
    printf("  RMS discretization error in Poisson solution = %g\n", rms_norm_diff(uek, ujk, nk));
    printf("  Walltime (MONOTONIC) = %.6f seconds\n", elapsed_time);

    printf("\n");
    printf("JACOBI_POISSON_1D_THREADS:\n");
    printf("  Normal end of execution.\n");

    free(xk);
    free(fk);
    free(uek);
    free(udk);
    free(ujk);

    return EXIT_SUCCESS;
}
