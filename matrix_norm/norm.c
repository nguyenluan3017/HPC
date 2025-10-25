#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <cblas.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#define ADD_FLAG(name, value) const char *const name = #value
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#define BENCHMARK(subroutine, result) \
    double result; \
    { \
        struct timespec ts_start; \
        struct timespec ts_end; \
        clock_gettime(CLOCK_MONOTONIC, &ts_start); \
        double start_time = ts_start.tv_sec + ts_start.tv_nsec * 1e-9; \
        subroutine; \
        clock_gettime(CLOCK_MONOTONIC, &ts_end); \
        double end_time = ts_end.tv_sec + ts_end.tv_nsec * 1e-9; \
        result = end_time - start_time; \
    }

#define DEFAULT_MATRIX_SIZE 1024
#define DEFAULT_MIN_VALUE 1
#define DEFAULT_MAX_VALUE 1000
#define DEFAULT_BLOCK_SIZE 512

ADD_FLAG(FLAG_HELP, --help);
ADD_FLAG(FLAG_MATRIX_SIZE, --matrix-size);
ADD_FLAG(FLAG_MIN_VALUE, --min-value);
ADD_FLAG(FLAG_MAX_VALUE, --max-value);
ADD_FLAG(FLAG_BLOCK_SIZE, --block-size);

typedef struct
{
    bool flag_help;
    size_t flag_matrix_size;
    int flag_min_value;
    int flag_max_value;
    size_t flag_block_size;
} args_t;

typedef struct
{
    size_t size;
    double *data;
} matrix_t;

void show_help(const char *program_name)
{
    printf("Usage:\n");
    printf("  %s [FLAGS]\n\n", program_name);
    printf("Description:\n");
    printf("  Computes the norm of the product of NxN dense matrices on p processors.\n\n");

    printf("Flags:\n");
    printf("  %s                Show this help message.\n", FLAG_HELP);
    printf("  %s <size>        Set matrix size (default: %d).\n", FLAG_MATRIX_SIZE, DEFAULT_MATRIX_SIZE);
    printf("  %s <value>       Set minimum random value (default: %d).\n", FLAG_MIN_VALUE, DEFAULT_MIN_VALUE);
    printf("  %s <value>       Set maximum random value (default: %d).\n", FLAG_MAX_VALUE, DEFAULT_MAX_VALUE);
    printf("  %s <value>       Set block size for serial multiplication. This must divide matrix size (default: %d).\n", FLAG_BLOCK_SIZE, DEFAULT_BLOCK_SIZE);

    printf("\nExamples:\n");
    printf("  %s --matrix-size 1024 --block-size 128\n", program_name);
    printf("  %s --matrix-size 512 --min-value 10 --max-value 1000\n", program_name);
    printf("  %s --help\n", program_name);
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
        args->flag_max_value
    );

    panic_unless(
        args->flag_matrix_size % args->flag_block_size == 0,
        "Block size (%d) must divide matrix size (%d).\n",
        args->flag_block_size,
        args->flag_matrix_size
    );
}

args_t args_parse(int argc, const char **argv)
{
    args_t args = {
        .flag_help = false,
        .flag_matrix_size = DEFAULT_MATRIX_SIZE,
        .flag_min_value = DEFAULT_MIN_VALUE,
        .flag_max_value = DEFAULT_MAX_VALUE,
        .flag_block_size = DEFAULT_BLOCK_SIZE,
    };

    if (argc == 1)
    {
        args.flag_help = true;
    }

    for (int i = 0; i < argc; i++)
    {
        if (strcmp(argv[i], FLAG_HELP) == 0)
        {
            args.flag_help = true;
            break;
        }
        else if (strcmp(argv[i], FLAG_MATRIX_SIZE) == 0)
        {
            panic_unless(i + 1 < argc, "Matrix size should be an unsigned integer.\n");
            args.flag_matrix_size = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_MIN_VALUE) == 0)
        {
            panic_unless(i + 1 < argc, "Min value must be an integer.\n");
            args.flag_min_value = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_MAX_VALUE) == 0)
        {
            panic_unless(i + 1 < argc, "Max value must be an integer.\n");
            args.flag_max_value = atoi(argv[i + 1]);
        }
        else if (strcmp(argv[i], FLAG_BLOCK_SIZE) == 0)
        {
            panic_unless(i + 1 < argc, "Block size must be an integer.\n");
            args.flag_block_size = atoi(argv[i + 1]);
        }
    }

    args_validate(&args);

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
    const double EPS = 1e-9;

    panic_unless(
        lhs->size == rhs->size, 
        "Only compare two square matrices of the same size, not %dx%d versus %dx%d",
        lhs->size, lhs->size,
        rhs->size, rhs->size
    );

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

void matrix_mult_serial(size_t block_size, matrix_t *lhs, matrix_t *rhs, matrix_t *result)
{
    panic_unless(
        lhs->size == rhs->size, 
        "I can only multiply two square matrices of the same size, not %dx%d multiply by %dx%d",
        lhs->size, lhs->size,
        rhs->size, rhs->size
    );

    const size_t N = lhs->size;
    
    for (size_t bi = 0; bi < N; bi += block_size)
    {
        for (size_t bj = 0; bj < N; bj += block_size)
        {
            for (size_t bk = 0; bk < N; bk += block_size)
            {
                const int i_end = MIN(bi + block_size, N);
                const int j_end = MIN(bj + block_size, N);
                const int k_end = MIN(bk + block_size, N);

                for (int k = bk; k < k_end; k++)
                {
                    for (int i = bi; i < i_end; i++)
                    {
                        register const double lhs_data = lhs->data[i * N + k];
                        register double *rhs_row = &rhs->data[k * N];
                        register double *result_row = &result->data[i * N];
                        register const int limit = j_end - ((j_end - bj) % 4);
                        register int j;

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
        "I can only multiply two square matrices of the same size, not %dx%d multiply by %dx%d",
        lhs->size, lhs->size,
        rhs->size, rhs->size
    );

    const size_t N = lhs->size;

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
        N
    );
}

void matrix_mult_pthread(matrix_t *lhs, matrix_t *rhs, matrix_t *result)
{
    
}

int main(int argc, const char **argv)
{
    args_t args = args_parse(argc, argv);
    srand(time(NULL));

    if (args.flag_help)
    {
        show_help(argv[0]);
    }
    else
    {
        matrix_t *A;
        matrix_t *B;
        matrix_t *C;
        matrix_t *D; 

        A = matrix_init(args.flag_matrix_size);
        B = matrix_init(args.flag_matrix_size);
        C = matrix_init(args.flag_matrix_size);
        D = matrix_init(args.flag_matrix_size);

        matrix_random(A, args.flag_min_value, args.flag_max_value);
        matrix_random(B, args.flag_min_value, args.flag_max_value);

        //puts("LHS:");
        //matrix_println(A);
        
        //puts("RHS:");
        //matrix_println(B);

        //puts("Serial result:");
        BENCHMARK(matrix_mult_serial(args.flag_block_size, A, B, C), serial_runtime);
        printf("serial_runtime = %f\n", serial_runtime);
        //matrix_println(C);
        
        //puts("Cblas result:");
        BENCHMARK(matrix_mult_cblas(A, B, D), cblas_runtime);
        printf("cblas_runtime = %f\n", cblas_runtime);
        //matrix_println(D);

        if (matrix_compare(C, D) != 0)
        {
            printf("Discrepancy in result!");
        }
        
        matrix_destroy(&A);
        matrix_destroy(&B);
        matrix_destroy(&C);
        matrix_destroy(&D);
    }
    return 0;
}
