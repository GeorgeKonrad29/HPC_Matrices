#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <semaphore.h>

#define MAX_IT 1000000

typedef struct {
    double sum_sq_res;
    double sum_sq_change;
} WorkerResult;

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

static int jacobi_poisson_1d_processes(int nk, double h2, const double *f, double *u, double tol, int nproc) {
    double *u_old = (double *)mmap(NULL, (size_t)nk * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    double *u_new = (double *)mmap(NULL, (size_t)nk * sizeof(double), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    WorkerResult *partials = (WorkerResult *)mmap(NULL, (size_t)nproc * sizeof(WorkerResult), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_t *sem_start = (sem_t *)mmap(NULL, (size_t)nproc * sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_t *sem_done = (sem_t *)mmap(NULL, (size_t)nproc * sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    int *command = (int *)mmap(NULL, sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);

    enum { CMD_UPDATE = 1, CMD_RESIDUAL = 2, CMD_EXIT = 3 };

    if (u_old == MAP_FAILED || u_new == MAP_FAILED || partials == MAP_FAILED ||
        sem_start == MAP_FAILED || sem_done == MAP_FAILED || command == MAP_FAILED) {
        fprintf(stderr, "Error: no se pudo reservar memoria compartida.\n");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < nk; i++) {
        u_old[i] = 0.0;
        u_new[i] = 0.0;
    }
    u_old[0] = u[0];
    u_old[nk - 1] = u[nk - 1];
    u_new[0] = u[0];
    u_new[nk - 1] = u[nk - 1];

    pid_t *pids = (pid_t *)malloc((size_t)nproc * sizeof(pid_t));

    if (!pids) {
        fprintf(stderr, "Error: no se pudo reservar memoria para procesos.\n");
        exit(EXIT_FAILURE);
    }

    for (int p = 0; p < nproc; p++) {
        if (sem_init(&sem_start[p], 1, 0) != 0 || sem_init(&sem_done[p], 1, 0) != 0) {
            fprintf(stderr, "Error: no se pudo inicializar semáforos compartidos.\n");
            exit(EXIT_FAILURE);
        }
    }

    *command = CMD_UPDATE;

    for (int p = 0; p < nproc; p++) {
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error: no se pudo crear proceso hijo en inicialización.\n");
            exit(EXIT_FAILURE);
        }

        if (pid == 0) {
            int start, end;
            compute_chunk_bounds(p, nproc, nk, &start, &end);

            int local_count = end - start + 1;
            double *u_old_local = NULL;
            double *u_new_local = NULL;

            if (local_count > 0) {
                u_old_local = (double *)malloc((size_t)(local_count + 2) * sizeof(double));
                u_new_local = (double *)malloc((size_t)local_count * sizeof(double));
                if (!u_old_local || !u_new_local) {
                    free(u_old_local);
                    free(u_new_local);
                    _exit(EXIT_FAILURE);
                }
            }

            while (1) {
                sem_wait(&sem_start[p]);

                if (*command == CMD_EXIT) {
                    break;
                }

                if (*command == CMD_UPDATE) {
                    if (local_count > 0) {
                        for (int t = 0; t < local_count + 2; t++) {
                            int g = (start - 1) + t;
                            u_old_local[t] = u_old[g];
                        }

                        for (int t = 0; t < local_count; t++) {
                            int i = start + t;
                            u_new_local[t] = 0.5 * (u_old_local[t] + u_old_local[t + 2] + h2 * f[i]);
                        }

                        for (int t = 0; t < local_count; t++) {
                            int i = start + t;
                            u_new[i] = u_new_local[t];
                        }
                    }
                } else if (*command == CMD_RESIDUAL) {
                    WorkerResult wr;
                    wr.sum_sq_res = 0.0;
                    wr.sum_sq_change = 0.0;

                    for (int i = start; i <= end; i++) {
                        double ri = (-u_new[i - 1] + 2.0 * u_new[i] - u_new[i + 1]) / h2 - f[i];
                        double du = u_new[i] - u_old[i];
                        wr.sum_sq_res += ri * ri;
                        wr.sum_sq_change += du * du;
                    }

                    partials[p] = wr;
                }

                sem_post(&sem_done[p]);
            }

            free(u_old_local);
            free(u_new_local);
            _exit(EXIT_SUCCESS);
        }

        pids[p] = pid;
    }

    int it = 0;
    double residual_rms = 0.0;

    while (1) {
        *command = CMD_UPDATE;
        for (int p = 0; p < nproc; p++) {
            sem_post(&sem_start[p]);
        }
        for (int p = 0; p < nproc; p++) {
            sem_wait(&sem_done[p]);
        }

        *command = CMD_RESIDUAL;
        for (int p = 0; p < nproc; p++) {
            sem_post(&sem_start[p]);
        }

        double total_res = 0.0;
        for (int p = 0; p < nproc; p++) {
            sem_wait(&sem_done[p]);
            total_res += partials[p].sum_sq_res;
        }

        it++;
        residual_rms = sqrt(total_res / (double)nk);

        if (residual_rms <= tol) {
            break;
        }

        /*if (it >= MAX_IT) {
            fprintf(stderr, "Advertencia: Jacobi alcanzó el máximo de iteraciones.\n");
            break;
        }*/

        for (int i = 1; i < nk - 1; i++) {
            u_old[i] = u_new[i];
        }
    }

    *command = CMD_EXIT;
    for (int p = 0; p < nproc; p++) {
        sem_post(&sem_start[p]);
    }

    for (int p = 0; p < nproc; p++) {
        int status = 0;
        waitpid(pids[p], &status, 0);
        if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
            fprintf(stderr, "Error: proceso hijo %d terminó anormalmente.\n", p);
                exit(EXIT_FAILURE);
        }
    }

    for (int i = 0; i < nk; i++) {
        u[i] = u_new[i];
    }

    free(pids);
    for (int p = 0; p < nproc; p++) {
        sem_destroy(&sem_start[p]);
        sem_destroy(&sem_done[p]);
    }
    munmap(command, sizeof(int));
    munmap(sem_start, (size_t)nproc * sizeof(sem_t));
    munmap(sem_done, (size_t)nproc * sizeof(sem_t));
    munmap(partials, (size_t)nproc * sizeof(WorkerResult));
    munmap(u_old, (size_t)nk * sizeof(double));
    munmap(u_new, (size_t)nk * sizeof(double));

    return it;
}

int main(int argc, char *argv[]) {
    int k = 5;
    int nproc = 4;

    if (argc > 1) {
        k = atoi(argv[1]);
    }
    if (argc > 2) {
        nproc = atoi(argv[2]);
    }
    double tol = 1.0e-6;
    if (argc > 3) {
        tol = atof(argv[3]);
        if (tol <= 0.0) {
            fprintf(stderr, "Error: Tolerance must be positive.\n");
            return EXIT_FAILURE;
        }
    }

    if (k < 1) {
        fprintf(stderr, "Error: K debe ser >= 1.\n");
        return EXIT_FAILURE;
    }
    if (nproc < 1) {
        fprintf(stderr, "Error: número de procesos debe ser >= 1.\n");
        return EXIT_FAILURE;
    }

    int nk = (1 << k) + 1;
    int max_proc = nk - 2;
    if (max_proc < 1) {
        max_proc = 1;
    }
    if (nproc > max_proc) {
        nproc = max_proc;
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
    printf("JACOBI_POISSON_1D_PROCESSES:\n");
    printf("  Use Jacobi iteration (fork) for the 1D Poisson equation.\n");

    solve_direct_tridiagonal(nk, h2, ua, ub, fk, udk);

    ujk[0] = ua;
    ujk[nk - 1] = ub;
    int it_num = jacobi_poisson_1d_processes(nk, h2, fk, ujk, tol, nproc);

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
    printf("  Processes used = %d\n", nproc);
    printf("  Tolerance for linear residual %g\n", tol);
    printf("  Number of Jacobi iterations required was %d\n", it_num);
    printf("  RMS Jacobi error in solution of linear system = %g\n", rms_norm_diff(udk, ujk, nk));
    printf("  RMS discretization error in Poisson solution = %g\n", rms_norm_diff(uek, ujk, nk));
    printf("  Walltime (MONOTONIC) = %.6f seconds\n", elapsed_time);

    printf("\n");
    printf("JACOBI_POISSON_1D_PROCESSES:\n");
    printf("  Normal end of execution.\n");

    free(xk);
    free(fk);
    free(uek);
    free(udk);
    free(ujk);

    return EXIT_SUCCESS;
}
