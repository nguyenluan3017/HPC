#include <stdio.h>
#include <stdbool.h>
#include <pthread.h>
#include <cblas.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>

#define ADD_FLAG(name, value) const char *const name = #value
#define DEFAULT_MATRIX_SIZE 1024
#define MAX_VALUE 10000

ADD_FLAG(FLAG_HELP, --help);
ADD_FLAG(FLAG_MATRIX_SIZE, --matrix-size);

typedef struct
{
    bool flag_help;
    size_t flag_matrix_size;
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

args_t args_parse(int argc, const char **argv)
{
    args_t args = {
        .flag_help = false,
        .flag_matrix_size = DEFAULT_MATRIX_SIZE,
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
    }

    return args;
}

matrix_t *matrix_init(size_t size)
{
    matrix_t *mat = (matrix_t *)calloc(1, sizeof(matrix_t));
    mat->size = size;
    mat->data = (double *)calloc(size * size, sizeof(double));
    return mat;
}

void matrix_random(matrix_t *mat)
{
    const size_t N = mat->size;
    for (size_t i = 0; i < N; i++)
    {
        for (size_t j = 0; j < N; j++)
        {
            mat->data[i * N + j] = (double)(rand() % MAX_VALUE);
        }
    }
}

void matrix_println(matrix_t *mat)
{
    const size_t N = mat->size;
    for (size_t i = 0; i < N; i++)
    {
        printf("[");
        for (size_t j = 0; j < N; j++)
        {
            printf("%8.3f", mat->data[i * N + j]);
        }
        printf("]\n");
    }
}

int main(int argc, const char **argv)
{
    args_t args = args_parse(argc, argv);
    if (args.flag_help)
    {
        show_help(argv[0]);
    }
    else
    {
        const size_t N = args.flag_matrix_size;
        matrix_t *mat = matrix_init(N);
        matrix_println(mat);
        free(mat);
    }
    return 0;
}
