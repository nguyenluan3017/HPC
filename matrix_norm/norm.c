#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <cblas.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <math.h>

#define ADD_FLAG(name, value) const char *const name = #value
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#define MIN(a,b) ((a) < (b) ? (a) : (b))

#define DEFAULT_MATRIX_SIZE 1024
#define DEFAULT_MIN_VALUE 1
#define DEFAULT_MAX_VALUE 10000000

ADD_FLAG(FLAG_HELP, --help);
ADD_FLAG(FLAG_MATRIX_SIZE, --matrix-size);
ADD_FLAG(FLAG_MIN_VALUE, --min-value);
ADD_FLAG(FLAG_MAX_VALUE, --max-value);

typedef struct
{
    bool flag_help;
    size_t flag_matrix_size;
    int flag_min_value;
    int flag_max_value;
} args_t;

typedef struct
{
    size_t size;
    double *data;
} matrix_t;

void show_help(const char *program_name)
{
    printf("%s [FLAGS]\n", program_name);
    printf("Computing the norm of the product of NxN dense matrices on p-processor.\n");
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
}

args_t args_parse(int argc, const char **argv)
{
    args_t args = {
        .flag_help = false,
        .flag_matrix_size = DEFAULT_MATRIX_SIZE,
        .flag_min_value = DEFAULT_MIN_VALUE,
        .flag_max_value = DEFAULT_MAX_VALUE,
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
    }
    return 0;
}
