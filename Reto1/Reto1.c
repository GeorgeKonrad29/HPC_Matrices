#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

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

static int jacobi_poisson_1d(int nk, double h2, const double *f, double *u, double tol) {

	double sum_sq_res = 0.0;
	double sum_sq_change = 0.0;
	double utemp=0.0;
	double utempminusone=0.0;
	int it = 0;
	while (1) {
		
		
		utempminusone = u[0];
		sum_sq_change = 0.0;
		for (int i = 1; i < nk - 1; i++) {
			utemp = u[i];
			u[i] = 0.5 * (utempminusone + u[i + 1] + h2 * f[i]);
			double du = u[i] - utemp;
			sum_sq_change += du * du;
			utempminusone = utemp;
		}

		
		sum_sq_res = 0.0;
		for (int i = 1; i < nk - 1; i++) {
			double ri = (-u[i - 1] + 2.0 * u[i] - u[i + 1]) / h2 - f[i];
			sum_sq_res += ri * ri;
		}

		it++;
		double residual_rms = sqrt(sum_sq_res / (double)nk);
		double change_rms = sqrt(sum_sq_change / (double)nk);

		if (residual_rms <= tol) {
			break;
		}

/*		if (it >= 1000000) {
			fprintf(stderr, "Advertencia: Jacobi alcanzó el máximo de iteraciones.\n");
			break;
		}*/
	}


	return it;
}

int main(int argc, char *argv[]) {
	int k = 5;
	if (argc > 1) {
		k = atoi(argv[1]);
		if (k < 1) {
			fprintf(stderr, "Error: K debe ser >= 1.\n");
			return EXIT_FAILURE;
		}
	}
	double tol = 1.0e-6;
	if (argc > 2) {
		tol = atof(argv[2]);
		if (tol <= 0.0) {
			fprintf(stderr, "Error: Tolerance must be positive.\n");
			return EXIT_FAILURE;
		}
	}
	if (argc > 3) {
		fprintf(stderr, "Advertencia: se ignorarán argumentos adicionales.\n");
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
	

	int nk = (1 << k) + 1;
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
	printf("JACOBI_POISSON_1D:\n");
	printf("  Use Jacobi iteration for the 1D Poisson equation.\n");

	solve_direct_tridiagonal(nk, h2, ua, ub, fk, udk);

	ujk[0] = ua;
	ujk[nk - 1] = ub;
	int it_num = jacobi_poisson_1d(nk, h2, fk, ujk, tol);

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
	printf("  Tolerance for linear residual %g\n", tol);
	printf("  Number of Jacobi iterations required was %d\n", it_num);
	printf("  RMS Jacobi error in solution of linear system = %g\n",
		   rms_norm_diff(udk, ujk, nk));
	printf("  RMS discretization error in Poisson solution = %g\n",
		   rms_norm_diff(uek, ujk, nk));
	printf("  Walltime (MONOTONIC) = %.6f seconds\n", elapsed_time);

	// Tabla de resultados comentada para evitar expansión
	/*
	printf("\n");
	printf("    I          X    U_Exact   U_Direct   U_Jacobi\n");
	printf("\n");
	for (int i = 0; i < nk; i++) {
		printf(" %4d %10.4f %10.4g %10.4g %10.4g\n",
			   i + 1, xk[i], uek[i], udk[i], ujk[i]);
	}
	printf("\n");
	*/
	

	printf("\n");
	printf("JACOBI_POISSON_1D:\n");
	printf("  Normal end of execution.\n");

	free(xk);
	free(fk);
	free(uek);
	free(udk);
	free(ujk);

	return EXIT_SUCCESS;
}
