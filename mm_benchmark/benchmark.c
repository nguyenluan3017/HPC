#include <assert.h>
#include <cblas.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MAX_SIZE 4096
#define min(a, b) ((a)>(b)?(b):(a))

const char *const ARG_BLOCK = "--block";
const char *const ARG_HELP = "--help";
const char *const ARG_SIZE = "--size";
const char *const ARG_VARIANT = "--variant";
const char *const ARG_VERBOSE = "--verbose";
const char *const ARG_VALUE_RANGE = "--value-range";

const char *const VARIANT_BLAS = "blas";
const char *const VARIANT_BLOCK = "block";
const char *const VARIANT_NAIVE = "naive";

typedef struct {
    bool flag_help;
    bool flag_verbose;
    char flag_variant[64];
    int flag_block;
    int flag_size;
    int value_min;
    int value_max;
} args_t;

typedef struct {
    int size;
    double mem[MAX_SIZE][MAX_SIZE];
} matrix_t;

void show_help(const char *prog_nam) {
    printf("Usage: %s [OPTIONS]\n\n", prog_nam);
    printf("Matrix multiplication program with different implementation variants.\n\n");
    printf("Options:\n");
    printf("  --help                 Show this help message and exit\n");
    printf("  --variant VARIANT      Specify the multiplication variant to use\n");
    printf("                         Available variants: naive, block, blas\n");
    printf("  --size SIZE            Size of the square matrices (positive integer, max %d)\n", MAX_SIZE);
    printf("  --verbose              Enable verbose output\n");
    printf("  --block BLOCK          Block size for block variant (positive integer)\n");
    printf("  --value-range MIN MAX  Specify the range of matrix values (default: 0 99)\n");
    printf("\n");
    printf("Variants:\n");
    printf("  naive                  Standard triple-loop matrix multiplication\n");
    printf("  block                  Block-based matrix multiplication (cache-friendly)\n");
    printf("  blas                   BLAS library implementation\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s --variant naive --size 100\n", prog_nam);
    printf("  %s --variant block --size 512 --block 64\n", prog_nam);
    printf("  %s --variant blas --size 512 --value-range 1 10\n", prog_nam);
    printf("  %s --help\n", prog_nam);
}

args_t args_parse(int argc, char *argv[]) {
    args_t ans = {
        .flag_help = false,
        .flag_verbose = false,
        .flag_variant = {0},
        .flag_block = 0,
        .flag_size = 0,
        .value_min = 0,
        .value_max = 99,
    };

    if (argc == 1) {
        ans.flag_help = true;
    } else {
        for (int i = 1; i < argc; i++) {
            if (strcmp(argv[i], ARG_HELP) == 0) {
                ans.flag_help = true;   
            } else if (strcmp(argv[i], ARG_VARIANT) == 0) {
                assert(i + 1 < argc);
                strncpy(ans.flag_variant, argv[i + 1], sizeof ans.flag_variant);
                i++;
            } else if (strcmp(argv[i], ARG_SIZE) == 0) {
                char ssize[64] = {0};
                assert(i + 1 < argc);
                strncpy(ssize, argv[i + 1], sizeof ssize);
                ans.flag_size = atoi(ssize);
                i++;
            } else if (strcmp(argv[i], ARG_VERBOSE) == 0) {
                ans.flag_verbose = true;
            } else if (strcmp(argv[i], ARG_BLOCK) == 0) {
                char bsize[64] = {0};
                assert(i + 1 < argc);
                strncpy(bsize, argv[i + 1], sizeof bsize);
                ans.flag_block = atoi(bsize);
                i++;
            } else if (strcmp(argv[i], ARG_VALUE_RANGE) == 0) {
                assert(i + 2 < argc);
                ans.value_min = atoi(argv[i + 1]);
                ans.value_max = atoi(argv[i + 2]);
                if (ans.value_min >= ans.value_max) {
                    fprintf(stderr, "Invalid range: MIN (%d) must be less than MAX (%d)\n", 
                           ans.value_min, ans.value_max);
                    exit(-1);
                }
                i += 2;
            }
        }
    }

    return ans;
}

double get_time() {
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return ts.tv_sec + ts.tv_nsec * 1e-9;
}

matrix_t *matrix_new(int N, int min_val, int max_val) {
    matrix_t *C = (matrix_t *)calloc(1, sizeof(matrix_t));

    C->size = N;
    int range = max_val - min_val + 1;
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C->mem[i][j] = (rand() % range) + min_val;
        }
    }

    return C;
}

void matrix_print(matrix_t *m) {
    const int N = m->size;
    printf("np.array([");
    for (int i = 0; i < N; i++) {
        printf("[");
        for (int j = 0; j < N; j++) {
            printf("%lf", m->mem[i][j]);
            if (j < N - 1) {
                printf(", ");
            }
        }
        printf("]");
        if (i < N - 1) {
            printf(",\n          ");
        }
    }
    printf("])\n");
}

matrix_t *matrix_mult_naive(matrix_t *A, matrix_t *B, double *runtime = NULL) {
    if (A->size != B->size) {
        return NULL;
    }

    const int N = A->size;
    matrix_t *C = (matrix_t *)calloc(1, sizeof(matrix_t));
    C->size = N;

    if (runtime != NULL) {
	*runtime = get_time();
    }
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            for (int k = 0; k < N; k++) {
                C->mem[i][j] += A->mem[i][k] * B->mem[k][j];
            }
        }
    }
    if (runtime != NULL) {
	    *runtime = get_time() - *runtime;
    }

    return C;
}

matrix_t *matrix_mult_block(matrix_t *A, matrix_t *B, int block_size, double *runtime = NULL) {
    if (A->size != B->size) {
        return NULL;
    }

    if (block_size <= 0) {
        fprintf(stderr, "Block size must be positive (receiving %d)\n", block_size);
        exit(-1);
    }

    const int N = A->size;
    matrix_t *C;
    const int BLOCK_COUNT = N / block_size;

    C = (matrix_t *)calloc(1, sizeof(matrix_t));
    C->size = N;

    if (N % block_size != 0) {
	    fprintf(stderr, "Block size must divide matrix size.");
	    exit(-1);
    }

    if (runtime != NULL) {
	    *runtime = get_time();
    } 
    for (int bi = 0; bi < N; bi += block_size) {
        for (int bj = 0; bj < N; bj += block_size) {
            for (int i = bi; i < min(bi + block_size, N); i++) {
                for (int j = bj; j < min(bj + block_size, N); j++) {
                    double sum = 0;
                    for (int k = 0; k < N; k++) {
                        sum += A->mem[i][k] * B->mem[k][j];
                    }
                    C->mem[i][j] += sum;
                }
            }
        }
    }
    if (runtime != NULL) {
	    *runtime = get_time() - *runtime;
    }

    return C;
}

matrix_t *matrix_mult_cblas(matrix_t *A, matrix_t *B, double *runtime = NULL) {
    const int N = A->size;

    double *A_d = (double *)calloc(N * N, sizeof(double));
    double *B_d = (double *)calloc(N * N, sizeof(double));
    double *C_d = (double *)calloc(N * N, sizeof(double));
    
    matrix_t *C = (matrix_t *)calloc(1, sizeof(matrix_t));
    C->size = N;

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            A_d[i * N + j] = A->mem[i][j];
            B_d[i * N + j] = B->mem[i][j];
        }
    }

    if (runtime == NULL) {
	    *runtime = get_time();
    }
    cblas_dgemm(
        CblasRowMajor,
        CblasNoTrans,
        CblasNoTrans,
        N,
        N,
        N,
        1.0,
        A_d,
        N,
        B_d,
        N,
        0.0,    
        C_d,
        N    
    );
    if (runtime == NULL) {
	    *runtime = get_time() - *runtime;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            C->mem[i][j] = C_d[i * N + j];
        }
    }

    free(A_d);
    free(B_d);
    free(C_d);

    return C;
}

void generate_matrices(bool verbose, int size, int min_val, int max_val, matrix_t **A, matrix_t **B) {
    if (verbose) {
        printf("Two matrices of size %dx%d with values in range [%d, %d]\n", size, size, min_val, max_val);
    }
    
    *A = matrix_new(size, min_val, max_val);
    if (verbose) {
        matrix_print(*A);
    }

    *B = matrix_new(size, min_val, max_val);
    if (verbose) {
        matrix_print(*B);
    }
}

void benchmark(args_t args) {
    matrix_t *A = NULL;
    matrix_t *B = NULL;
    matrix_t *C = NULL;

    generate_matrices(
        args.flag_verbose, 
        args.flag_size, 
        args.value_min, 
        args.value_max, 
        &A, 
        &B
    );

    if (strcmp(args.flag_variant, VARIANT_NAIVE) == 0) {
        C = matrix_mult_naive(A, B);
    } else if (strcmp(args.flag_variant, VARIANT_BLOCK) == 0) {
        C = matrix_mult_block(A, B, args.flag_block);
    } else if (strcmp(args.flag_variant, VARIANT_BLAS) == 0) {
        C = matrix_mult_cblas(A, B);
    } else {
        fprintf(stderr, "Unsupported variant: %s\n", args.flag_variant);
        exit(-1);
    }

    if (C == NULL) {
        fprintf(stderr, "Can't perform matrix multiplication for variant '%s'\n", args.flag_variant);
    } else if (args.flag_verbose) {
        printf("The result matrix is:\n");
        matrix_print(C);
    }

    if (A != NULL) {
        free(A);
    }
    
    if (B != NULL) {
        free(B);
    }

    if (C != NULL) {
        free(C);
    }
}

int main(int argc, char *argv[]) {
    args_t args = args_parse(argc, argv);

    if (args.flag_help) {
        show_help(argv[0]);
    } else {
        if (args.flag_size <= 0) {
            fprintf(stderr, "Size of matrix should not be positive (but receiving %d)\n", args.flag_size);
            return -1;
        }
        srand(time(0));
        benchmark(args);
    }

    return 0;
}
