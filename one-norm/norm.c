#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <cblas.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define FLAG_HELP "--help"
#define FLAG_MATRIX_SIZE "--matrix-size"
#define FLAG_MIN_VALUE "--min-value"
#define FLAG_MAX_VALUE "--max-value"
#define FLAG_BLOCK_SIZE "--block-size"
#define FLAG_NUMBER_OF_THREADS "--number-of-threads"
#define FLAG_REPEATS "--repeats"
#define FLAG_IMPL "--impl"

#define IMPL_NAIVE "naive"
#define IMPL_SERIAL "serial"
#define IMPL_CBLAS "cblas"
#define IMPL_THREADED "threaded"

#define EPS 1e-9
#define DEFAULT_MATRIX_SIZE 1024
#define DEFAULT_MIN_VALUE 1
#define DEFAULT_MAX_VALUE 1000
#define DEFAULT_BLOCK_SIZE 512
#define DEFAULT_NUM_THREADS 4
#define DEFAULT_REPEATS 1
#define DEFAULT_IMPL IMPL_CBLAS

#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MEASURE_RUNTIME(subroutine, result)                            \
    {                                                                  \
        struct timespec ts_start;                                      \
        struct timespec ts_end;                                        \
        clock_gettime(CLOCK_MONOTONIC, &ts_start);                     \
        double start_time = ts_start.tv_sec + ts_start.tv_nsec * 1e-9; \
        subroutine;                                                    \
        clock_gettime(CLOCK_MONOTONIC, &ts_end);                       \
        double end_time = ts_end.tv_sec + ts_end.tv_nsec * 1e-9;       \
        result = end_time - start_time;                                \
    }

typedef struct args_t
{
    bool flag_help;
    size_t flag_matrix_size;
    int flag_min_value;
    int flag_max_value;
    size_t flag_block_size;
    size_t flag_number_of_threads;
    size_t flag_repeats;
    const char *flag_impl;
} args_t;

typedef struct matrix_t
{
    size_t size;
    double *data;
} matrix_t;

typedef struct matrix_mult_worker_params_t
{
    matrix_t *lhs;
    matrix_t *rhs;
    double *result;
    size_t block_start_index;
    size_t block_end_index;
    size_t block_size;
} matrix_mult_worker_params_t;

typedef struct matrix_norm_worker_params_t
{
    matrix_t *mat;
    long double max_sum;
    size_t block_size;
    size_t start_index;
    size_t end_index;
} matrix_norm_worker_params_t;

typedef struct benchmark_result_t
{
    double benchmark_runtime;
    double norm_runtime;
    size_t block_size;
    size_t num_repeats;
    size_t num_threads;
    size_t matrix_size;
    const char *impl;
} benchmark_result_t;

void show_help(const char *program_name)
{
    printf("Usage:\n");
    printf("  %s [FLAGS]\n\n", program_name);

    printf("Description:\n");
    printf("  Computes the norm of the product of NxN dense matrices using various algorithms\n");
    printf("  including naive, serial blocked, CBLAS, and parallel pthread implementations.\n\n");

    printf("Flags:\n");
    printf("  %-25s Show this help message.\n", FLAG_HELP);
    printf("  %-25s Set matrix size (default: %d).\n", FLAG_MATRIX_SIZE, DEFAULT_MATRIX_SIZE);
    printf("  %-25s Set minimum random value (default: %d).\n", FLAG_MIN_VALUE, DEFAULT_MIN_VALUE);
    printf("  %-25s Set maximum random value (default: %d).\n", FLAG_MAX_VALUE, DEFAULT_MAX_VALUE);
    printf("  %-25s Set block size for serial multiplication.\n", FLAG_BLOCK_SIZE);
    printf("  %-25s This must divide matrix size (default: %d).\n", "", DEFAULT_BLOCK_SIZE);
    printf("  %-25s Set number of threads for parallel computation\n", FLAG_NUMBER_OF_THREADS);
    printf("  %-25s (default: %d).\n", "", DEFAULT_NUM_THREADS);
    printf("  %-25s Set number of benchmark repetitions\n", FLAG_REPEATS);
    printf("  %-25s (default: %d).\n", "", DEFAULT_REPEATS);
    printf("  %-25s Set implementation to use:\n", FLAG_IMPL);
    printf("  %-25s %s, %s, %s, %s (default: %s).\n", "", IMPL_NAIVE, IMPL_SERIAL, IMPL_CBLAS, IMPL_THREADED, DEFAULT_IMPL);

    printf("\nImplementations:\n");
    printf("  %-15s Basic O(n³) triple-nested loop matrix multiplication.\n", IMPL_NAIVE);
    printf("  %-15s Cache-optimized blocked matrix multiplication using\n", IMPL_SERIAL);
    printf("  %-15s the specified block size. Requires --block-size.\n", "");
    printf("  %-15s High-performance BLAS library implementation using\n", IMPL_CBLAS);
    printf("  %-15s optimized assembly routines (OpenBLAS).\n", "");
    printf("  %-15s Multi-threaded blocked matrix multiplication using\n", IMPL_THREADED);
    printf("  %-15s pthreads. Requires --block-size and --number-of-threads.\n", "");

    printf("\nConstraints:\n");
    printf("  - Matrix size must be positive\n");
    printf("  - Min value must be ≤ max value\n");
    printf("  - Block size must evenly divide matrix size\n");
    printf("  - Number of threads must be ≤ matrix size\n");
    printf("  - Number of repeats must be > 0\n");
    printf("  - Serial and threaded implementations require valid block size\n");
    printf("  - Threaded implementation requires valid number of threads\n");

    printf("\nExamples:\n");
    printf("  %s --matrix-size 1024 --impl naive\n", program_name);
    printf("  %s --matrix-size 1024 --block-size 128 --impl serial\n", program_name);
    printf("  %s --matrix-size 512 --impl cblas --repeats 3\n", program_name);
    printf("  %s --impl threaded --number-of-threads 8 --block-size 256\n", program_name);
    printf("  %s --matrix-size 2048 --min-value 1 --max-value 100 --repeats 5\n", program_name);
    printf("  %s --help\n", program_name);

    printf("\nNotes:\n");
    printf("  - Block size affects cache performance; try powers of 2 (64, 128, 256, 512)\n");
    printf("  - Number of threads should typically match your CPU cores\n");
    printf("  - Larger matrices will show more significant performance differences\n");
    printf("  - Use repeats > 1 for more accurate timing measurements\n");
    printf("  - CBLAS implementation is typically the fastest for large matrices\n");
    printf("  - Threaded implementation may have overhead for small matrices\n");
}

void panic_unless(bool predicate, const char *fmt, ...)
{
    if (predicate)
    {
        return;
    }

    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    abort();
}

void args_validate(args_t *args)
{
    panic_unless(
        args->flag_min_value <= args->flag_max_value,
        "Invalid value interval of %d .. %d\n",
        args->flag_min_value,
        args->flag_max_value);

    panic_unless(
        args->flag_matrix_size % args->flag_block_size == 0,
        "Block size (%d) must divide matrix size (%d).\n",
        args->flag_block_size,
        args->flag_matrix_size);

    panic_unless(
        args->flag_number_of_threads <= args->flag_matrix_size,
        "The number of threads (%d) should be no more than the matrix size (%d)\n",
        args->flag_number_of_threads,
        args->flag_matrix_size);

    panic_unless(
        args->flag_repeats > 0,
        "Number of repeats (%d) must be greater than 0\n",
        args->flag_repeats);

    panic_unless(
        strcmp(args->flag_impl, IMPL_NAIVE) == 0 ||
            strcmp(args->flag_impl, IMPL_SERIAL) == 0 ||
            strcmp(args->flag_impl, IMPL_CBLAS) == 0 ||
            strcmp(args->flag_impl, IMPL_THREADED) == 0,
        "Invalid implementation '%s'. Valid options: %s, %s, %s, %s\n",
        args->flag_impl, IMPL_NAIVE, IMPL_SERIAL, IMPL_CBLAS, IMPL_THREADED);

    if (strcmp(args->flag_impl, IMPL_SERIAL) == 0 || strcmp(args->flag_impl, IMPL_THREADED) == 0)
    {
        panic_unless(
            args->flag_block_size > 0,
            "Implementation '%s' requires a valid block size (current: %d)\n",
            args->flag_impl,
            args->flag_block_size);
    }
}

args_t *args_parse(int argc, const char **argv)
{
    args_t *args;

    args = (args_t *)calloc(1, sizeof(args_t));
    args->flag_help = false;
    args->flag_matrix_size = DEFAULT_MATRIX_SIZE;
    args->flag_min_value = DEFAULT_MIN_VALUE;
    args->flag_max_value = DEFAULT_MAX_VALUE;
    args->flag_block_size = DEFAULT_BLOCK_SIZE;
    args->flag_number_of_threads = DEFAULT_NUM_THREADS;
    args->flag_repeats = DEFAULT_REPEATS;
    args->flag_impl = DEFAULT_IMPL;

    if (argc == 1)
    {
        args->flag_help = true;
    }

    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], FLAG_HELP) == 0)
        {
            args->flag_help = true;
            break;
        }
        else if (strcmp(argv[i], FLAG_MATRIX_SIZE) == 0)
        {
            panic_unless(i + 1 < argc, "Matrix size should be an unsigned integer.\n");
            args->flag_matrix_size = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_MIN_VALUE) == 0)
        {
            panic_unless(i + 1 < argc, "Min value must be an integer.\n");
            args->flag_min_value = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_MAX_VALUE) == 0)
        {
            panic_unless(i + 1 < argc, "Max value must be an integer.\n");
            args->flag_max_value = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_BLOCK_SIZE) == 0)
        {
            panic_unless(i + 1 < argc, "Block size must be an integer.\n");
            args->flag_block_size = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_NUMBER_OF_THREADS) == 0)
        {
            panic_unless(i + 1 < argc, "The number of threads must be an unsigned integer.\n");
            args->flag_number_of_threads = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_REPEATS) == 0)
        {
            panic_unless(i + 1 < argc, "Number of repeats must be an unsigned integer.\n");
            args->flag_repeats = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_IMPL) == 0)
        {
            panic_unless(i + 1 < argc, "Implementation must be specified.\n");
            args->flag_impl = argv[i + 1];
        }
    }

    args_validate(args);
    return args;
}

matrix_t *matrix_init(size_t size)
{
    matrix_t *mat = (matrix_t *)calloc(1, sizeof(matrix_t));
    mat->size = size;
    mat->data = (double *)calloc(size * size, sizeof(double));
    return mat;
}

void matrix_destroy(matrix_t **mat)
{
    free((*mat)->data);
    free(*mat);
    *mat = NULL;
}

void matrix_random(matrix_t *mat, int min_value, int max_value)
{
    const size_t N = mat->size;
    for (size_t i = 0; i < N; i++)
    {
        for (size_t j = 0; j < N; j++)
        {
            mat->data[i * N + j] = min_value + rand() % (max_value - min_value + 1);
        }
    }
}

int matrix_compare(matrix_t *lhs, matrix_t *rhs)
{
    const size_t N = lhs->size;

    panic_unless(
        lhs->size == rhs->size,
        "Only compare two square matrices of the same size, not %dx%d versus %dx%d\n",
        lhs->size, lhs->size,
        rhs->size, rhs->size);

    for (size_t i = 0; i < N; i++)
    {
        for (size_t j = 0; j < N; j++)
        {
            const double diff = lhs->data[i * N + j] - rhs->data[i * N + j];
            if (fabs(diff) > EPS)
            {
                return diff < 0 ? -1 : 1;
            }
        }
    }

    return 0;
}

void matrix_println(matrix_t *mat)
{
    const size_t N = mat->size;
    size_t *col_size;
    char **col_fmt;
    char buf[128];
    size_t buflen;

    col_size = (size_t *)calloc(N, sizeof(size_t));
    col_fmt = (char **)calloc(N, sizeof(char *));

    for (size_t i = 0; i < N; i++)
    {
        for (size_t j = 0; j < N; j++)
        {
            sprintf(buf, "%.0f", mat->data[i * N + j]);
            buflen = strlen(buf);
            col_size[j] = MAX(col_size[j], buflen);
        }
    }

    for (size_t j = 0; j < N; j++)
    {
        col_fmt[j] = (char *)calloc(128, sizeof(char));
        sprintf(col_fmt[j], "%%%lu.0f", col_size[j] + 2);
    }

    for (size_t i = 0; i < N; i++)
    {
        printf("[");
        for (size_t j = 0; j < N; j++)
        {
            printf(col_fmt[j], mat->data[i * N + j]);
        }
        printf("]\n");
    }

    for (size_t i = 0; i < N; i++)
    {
        free(col_fmt[i]);
    }
    free(col_fmt);
    free(col_size);
}

void matrix_mult_naive(matrix_t *lhs, matrix_t *rhs, matrix_t *result)
{
    panic_unless(
        lhs->size == rhs->size,
        "I can only multiply two square matrices of the same size, not %dx%d multiply by %dx%d\n",
        lhs->size, lhs->size,
        rhs->size, rhs->size);

    const size_t N = lhs->size;
    
    // Zero out the result matrix
    memset(result->data, 0, N * N * sizeof(double));

    for (int i = 0; i < N; i++)
    {
        for (int j = 0; j < N; j++)
        {
            // Remove this line since we've already zeroed the matrix
            // result->data[i * N + j] = 0.0;
            for (int k = 0; k < N; k++)
            {
                result->data[i * N + j] += lhs->data[i * N + k] * rhs->data[k * N + j];
            }
        }
    }
}

void matrix_mult_serial(size_t block_size, matrix_t *lhs, matrix_t *rhs, matrix_t *result)
{
    panic_unless(
        lhs->size == rhs->size,
        "I can only multiply two square matrices of the same size, not %dx%d multiply by %dx%d\n",
        lhs->size, lhs->size,
        rhs->size, rhs->size);

    const size_t N = lhs->size;
    
    // Zero out the result matrix
    memset(result->data, 0, N * N * sizeof(double));

    for (size_t bi = 0; bi < N; bi += block_size)
    {
        for (size_t bj = 0; bj < N; bj += block_size)
        {
            for (size_t bk = 0; bk < N; bk += block_size)
            {
                const size_t i_end = MIN(bi + block_size, N);
                const size_t j_end = MIN(bj + block_size, N);
                const size_t k_end = MIN(bk + block_size, N);

                for (size_t k = bk; k < k_end; k++)
                {
                    for (size_t i = bi; i < i_end; i++)
                    {
                        register const double lhs_data = lhs->data[i * N + k];
                        register double *rhs_row = &rhs->data[k * N];
                        register double *result_row = &result->data[i * N];
                        register const size_t limit = j_end - ((j_end - bj) % 4);
                        register size_t j;

                        for (j = bj; j < limit; j += 4)
                        {
                            result_row[j] += lhs_data * rhs_row[j];
                            result_row[j + 1] += lhs_data * rhs_row[j + 1];
                            result_row[j + 2] += lhs_data * rhs_row[j + 2];
                            result_row[j + 3] += lhs_data * rhs_row[j + 3];
                        }

                        for (; j < j_end; j++)
                        {
                            result_row[j] += lhs_data * rhs_row[j];
                        }
                    }
                }
            }
        }
    }
}

void matrix_mult_cblas(matrix_t *lhs, matrix_t *rhs, matrix_t *result)
{
    panic_unless(
        lhs->size == rhs->size,
        "I can only multiply two square matrices of the same size, not %dx%d multiply by %dx%d\n",
        lhs->size, lhs->size,
        rhs->size, rhs->size);

    const size_t N = lhs->size;

    // Zero out the result matrix (though cblas_dgemm with beta=0.0 should handle this)
    memset(result->data, 0, N * N * sizeof(double));

    cblas_dgemm(
        CblasRowMajor,
        CblasNoTrans,
        CblasNoTrans,
        N,
        N,
        N,
        1.0,
        lhs->data,
        N,
        rhs->data,
        N,
        0.0,
        result->data,
        N);
}

void *matrix_mult_worker(void *param)
{
    matrix_mult_worker_params_t *worker_params = (matrix_mult_worker_params_t *)param;
    matrix_t *lhs = worker_params->lhs;
    matrix_t *rhs = worker_params->rhs;
    double *result = worker_params->result;
    const size_t N = lhs->size;
    const size_t block_size = worker_params->block_size;
    const size_t bi = worker_params->block_start_index;

    for (size_t bj = 0; bj < N; bj += block_size)
    {
        for (size_t bk = 0; bk < N; bk += block_size)
        {
            const size_t i_end = worker_params->block_end_index;
            const size_t j_end = MIN(bj + block_size, N);
            const size_t k_end = MIN(bk + block_size, N);

            for (size_t k = bk; k < k_end; k++)
            {
                for (size_t i = bi; i < i_end; i++)
                {
                    register const double lhs_data = lhs->data[i * N + k];
                    register double *rhs_row = &rhs->data[k * N];
                    register double *result_row = &result[i * N];
                    register const size_t limit = j_end - ((j_end - bj) % 4);
                    register size_t j;

                    for (j = bj; j < limit; j += 4)
                    {
                        result_row[j] += lhs_data * rhs_row[j];
                        result_row[j + 1] += lhs_data * rhs_row[j + 1];
                        result_row[j + 2] += lhs_data * rhs_row[j + 2];
                        result_row[j + 3] += lhs_data * rhs_row[j + 3];
                    }

                    for (; j < j_end; j++)
                    {
                        result_row[j] += lhs_data * rhs_row[j];
                    }
                }
            }
        }
    }

    pthread_exit(NULL);
}

void matrix_mult_threaded(size_t num_threads, size_t block_size, matrix_t *lhs, matrix_t *rhs, matrix_t *result)
{
    const size_t N = lhs->size;
    const size_t PARTITION_SIZE = (size_t)ceil((double)N / (double)num_threads);
    pthread_t *threads;
    matrix_mult_worker_params_t *worker_params;
    pthread_mutex_t *mutex;

    panic_unless(
        lhs->size == rhs->size,
        "I can only multiply two square matrices of the same size, not %dx%d multiply by %dx%d\n",
        lhs->size, lhs->size,
        rhs->size, rhs->size);

    // Zero out the result matrix before threading
    memset(result->data, 0, N * N * sizeof(double));

    threads = (pthread_t *)calloc(num_threads, sizeof(pthread_t));
    worker_params = (matrix_mult_worker_params_t *)calloc(num_threads, sizeof(matrix_mult_worker_params_t));
    mutex = (pthread_mutex_t *)calloc(1, sizeof(pthread_mutex_t));
    
    // Initialize mutex
    pthread_mutex_init(mutex, NULL);

    for (size_t i = 0; i < num_threads; i++)
    {
        worker_params[i].lhs = lhs;
        worker_params[i].rhs = rhs;
        worker_params[i].result = (double *)calloc(N * N, sizeof(double));
        worker_params[i].block_start_index = i * PARTITION_SIZE;
        worker_params[i].block_end_index = MIN((i + 1) * PARTITION_SIZE, N);
        worker_params[i].block_size = block_size;        

        pthread_create(
            &threads[i],
            NULL,
            matrix_mult_worker,
            (void *)&worker_params[i]);
    }

    for (size_t i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
        for (size_t j = 0; j < N * N; j++)
        {
            result->data[j] += worker_params[i].result[j];
        }
        free(worker_params[i].result);
        worker_params[i].result = NULL;
    }

    free(threads);
    free(worker_params);
    pthread_mutex_destroy(mutex);
    free(mutex);
}

void *matrix_norm_worker(void *params)
{
    matrix_norm_worker_params_t *worker_params = (matrix_norm_worker_params_t *)params;
    const size_t N = worker_params->mat->size;
    const size_t start_index = worker_params->start_index;
    const size_t end_index = worker_params->end_index;
    const size_t block_size = worker_params->block_size;
    double *data = worker_params->mat->data;
    register long double row_sum;
    register double *row;
    register size_t j_end;
    register size_t j;
    long double max_row_sum = 0.0;

    for (size_t i = start_index; i < end_index; i++)
    {
        row = &data[i * N];
        row_sum = 0.0;
        for (size_t bj = 0; bj < N; bj += block_size)
        {
            j_end = MIN(bj + block_size, N);
            for (j = bj; j < j_end; j++)
            {
                row_sum += fabsl(row[j]);
            }
        }
        max_row_sum = MAX(max_row_sum, row_sum);
    }

    worker_params->max_sum = max_row_sum;

    pthread_exit(NULL);
}

long double matrix_norm_serial(size_t block_size, matrix_t *mat)
{
    size_t N;
    double *data;
    register long double row_sum;
    register double *row;
    register size_t j_end;
    register size_t j;
    long double max_row_sum;
    size_t i, bj;

    N = mat->size;
    data = mat->data;
    max_row_sum = 0.0;

    for (i = 0; i < N; i++)
    {
        row = &data[i * N];
        row_sum = 0.0;
        for (bj = 0; bj < N; bj += block_size)
        {
            j_end = MIN(bj + block_size, N);
            for (j = bj; j < j_end; j++)
            {
                row_sum += fabsl(row[j]);
            }
        }
        max_row_sum = MAX(max_row_sum, row_sum);
    }

    return max_row_sum;
}

long double matrix_norm_threaded(size_t num_threads, size_t block_size, matrix_t *mat)
{
    const size_t N = mat->size;
    const size_t PARTITION_SIZE = (size_t)ceil((double)N / (double)num_threads);
    pthread_t *threads;
    matrix_norm_worker_params_t *worker_params;
    long double *sums;
    long double result;

    threads = (pthread_t *)calloc(num_threads, sizeof(pthread_t));
    worker_params = (matrix_norm_worker_params_t *)calloc(num_threads, sizeof(matrix_norm_worker_params_t));
    sums = (long double *)calloc(num_threads, sizeof(long double));

    for (size_t i = 0; i < num_threads; i++)
    {
        worker_params[i].mat = mat;
        worker_params[i].start_index = i * PARTITION_SIZE;
        worker_params[i].end_index = MIN((i + 1) * PARTITION_SIZE, N);
        worker_params[i].block_size = block_size;
        worker_params[i].max_sum = 0;

        pthread_create(
            &threads[i],
            NULL,
            matrix_norm_worker,
            (void *)&worker_params[i]);
    }

    result = 0;
    for (size_t i = 0; i < num_threads; i++)
    {
        pthread_join(threads[i], NULL);
        result = MAX(result, worker_params[i].max_sum);
    }

    free(threads);
    free(worker_params);
    free(sums);

    return result;
}

void write_result_in_yaml(FILE *file, size_t num_results, benchmark_result_t *results)
{
    size_t i;
    double avg_benchmark_runtime, avg_norm_runtime;
    double min_benchmark_runtime, max_benchmark_runtime;
    double min_norm_runtime, max_norm_runtime;

    panic_unless(file != NULL, "File could not be null");

    avg_benchmark_runtime = 0.0;
    avg_norm_runtime = 0.0;
    min_benchmark_runtime = results[0].benchmark_runtime;
    max_benchmark_runtime = results[0].benchmark_runtime;
    min_norm_runtime = results[0].norm_runtime;
    max_norm_runtime = results[0].norm_runtime;

    /* Calculate statistics */
    for (i = 0; i < num_results; i++)
    {
        avg_benchmark_runtime += results[i].benchmark_runtime;
        avg_norm_runtime += results[i].norm_runtime;

        min_benchmark_runtime = MIN(min_benchmark_runtime, results[i].benchmark_runtime);
        max_benchmark_runtime = MAX(max_benchmark_runtime, results[i].benchmark_runtime);
        min_norm_runtime = MIN(min_norm_runtime, results[i].norm_runtime);
        max_norm_runtime = MAX(max_norm_runtime, results[i].norm_runtime);
    }

    avg_benchmark_runtime /= num_results;
    avg_norm_runtime /= num_results;

    /* Write YAML header */
    fprintf(file, "benchmark_results:\n");
    fprintf(file, "  metadata:\n");
    fprintf(file, "    implementation: \"%s\"\n", results[0].impl);
    fprintf(file, "    matrix_size: %zu\n", results[0].matrix_size);
    fprintf(file, "    block_size: %zu\n", results[0].block_size);
    fprintf(file, "    num_threads: %zu\n", results[0].num_threads);
    fprintf(file, "    num_repeats: %zu\n", results[0].num_repeats);
    fprintf(file, "    timestamp: %ld\n", time(NULL));

    /* Write statistics */
    fprintf(file, "  statistics:\n");
    fprintf(file, "    multiplication:\n");
    fprintf(file, "      average_time: %.9f\n", avg_benchmark_runtime);
    fprintf(file, "      min_time: %.9f\n", min_benchmark_runtime);
    fprintf(file, "      max_time: %.9f\n", max_benchmark_runtime);
    fprintf(file, "    norm_computation:\n");
    fprintf(file, "      average_time: %.9f\n", avg_norm_runtime);
    fprintf(file, "      min_time: %.9f\n", min_norm_runtime);
    fprintf(file, "      max_time: %.9f\n", max_norm_runtime);
    fprintf(file, "    total:\n");
    fprintf(file, "      average_time: %.9f\n", avg_benchmark_runtime + avg_norm_runtime);
    fprintf(file, "      min_time: %.9f\n", min_benchmark_runtime + min_norm_runtime);
    fprintf(file, "      max_time: %.9f\n", max_benchmark_runtime + max_norm_runtime);

    /* Write individual run results */
    fprintf(file, "  individual_runs:\n");
    for (i = 0; i < num_results; i++)
    {
        fprintf(file, "    - run: %zu\n", i + 1);
        fprintf(file, "      multiplication_time: %.9f\n", results[i].benchmark_runtime);
        fprintf(file, "      norm_time: %.9f\n", results[i].norm_runtime);
        fprintf(file, "      total_time: %.9f\n", results[i].benchmark_runtime + results[i].norm_runtime);
    }
}

void benchmark(size_t num_repeats, size_t num_threads, size_t matrix_size, size_t block_size, int min_value, int max_value, const char *impl, benchmark_result_t *results)
{
    const bool is_naive = strcmp(impl, IMPL_NAIVE) == 0;
    const bool is_cblas = strcmp(impl, IMPL_CBLAS) == 0;
    const bool is_serial = strcmp(impl, IMPL_SERIAL) == 0;
    const bool is_threaded = strcmp(impl, IMPL_THREADED) == 0;
    matrix_t *A, *B, *C, *expected_mult_result;
    size_t i;
    long double mat_norm, expected_norm;

    A = matrix_init(matrix_size);
    B = matrix_init(matrix_size);
    C = matrix_init(matrix_size);
    expected_mult_result = matrix_init(matrix_size);

    matrix_random(A, min_value, max_value);
    matrix_random(B, min_value, max_value);

    matrix_mult_cblas(A, B, expected_mult_result);

    mat_norm = 0.0;
    expected_norm = 0.0;

    if (is_naive)
    {
        for (i = 0; i < num_repeats; i++)
        {
            MEASURE_RUNTIME(matrix_mult_naive(A, B, C), results[i].benchmark_runtime);
            MEASURE_RUNTIME(mat_norm = matrix_norm_serial(block_size, C), results[i].norm_runtime);
            results[i].block_size = block_size;
            results[i].impl = impl;
            results[i].matrix_size = matrix_size;
            results[i].num_repeats = num_repeats;
            results[i].num_threads = 1;
        }
    }
    else if (is_cblas)
    {
        for (i = 0; i < num_repeats; i++)
        {
            MEASURE_RUNTIME(matrix_mult_cblas(A, B, C), results[i].benchmark_runtime);
            MEASURE_RUNTIME(mat_norm = matrix_norm_serial(block_size, C), results[i].norm_runtime);
            results[i].block_size = block_size;
            results[i].impl = impl;
            results[i].matrix_size = matrix_size;
            results[i].num_repeats = num_repeats;
            results[i].num_threads = 1;
        }
    }
    else if (is_serial)
    {
        for (i = 0; i < num_repeats; i++)
        {
            MEASURE_RUNTIME(matrix_mult_serial(block_size, A, B, C), results[i].benchmark_runtime);
            MEASURE_RUNTIME(mat_norm = matrix_norm_serial(block_size, C), results[i].norm_runtime);
            results[i].block_size = block_size;
            results[i].impl = impl;
            results[i].matrix_size = matrix_size;
            results[i].num_repeats = num_repeats;
            results[i].num_threads = 1;
        }
    }
    else if (is_threaded)
    {
        for (i = 0; i < num_repeats; i++)
        {
            MEASURE_RUNTIME(matrix_mult_threaded(num_threads, block_size, A, B, C), results[i].benchmark_runtime);
            MEASURE_RUNTIME(mat_norm = matrix_norm_threaded(num_threads, block_size, C), results[i].norm_runtime);
            results[i].block_size = block_size;
            results[i].impl = impl;
            results[i].matrix_size = matrix_size;
            results[i].num_repeats = num_repeats;
            results[i].num_threads = num_threads;
        }
    }

    panic_unless(
        matrix_compare(C, expected_mult_result) == 0,
        "Discrepency in matrix multiplication results\n");

    if (is_threaded)
    {
        expected_norm = matrix_norm_serial(block_size, expected_mult_result);
    }
    else
    {
        expected_norm = matrix_norm_threaded(num_threads, block_size, expected_mult_result);
    }
    panic_unless(
        fabsl(expected_norm - mat_norm) < EPS,
        "Incorrect matrix norm estimation (expected: %Lf, actual: %Lf).",
        expected_norm,
        mat_norm);

    matrix_destroy(&A);
    matrix_destroy(&B);
    matrix_destroy(&C);
    matrix_destroy(&expected_mult_result);
}

int main(int argc, const char **argv)
{
    args_t *args;
    benchmark_result_t *results;

    args = args_parse(argc, argv);
    srand(time(NULL));

    if (args->flag_help)
    {
        show_help(argv[0]);
    }
    else
    {
        results = (benchmark_result_t *)calloc(args->flag_repeats, sizeof(benchmark_result_t));

        benchmark(
            args->flag_repeats,
            args->flag_number_of_threads,
            args->flag_matrix_size,
            args->flag_block_size,
            args->flag_min_value,
            args->flag_max_value,
            args->flag_impl,
            results);

        write_result_in_yaml(stdout, args->flag_repeats, results);

        free(results);
    }

    free(args);
    return 0;
}
