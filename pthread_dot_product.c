#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>

#define min(a, b) ((a) > (b) ? (b) : (a))
#define N ((size_t) 100000)
#define SEGLEN (10000)
#define NTHREAD ((size_t)(N / SEGLEN) + (N % SEGLEN ? 1 : 0))

typedef struct
{
    double *x;
    double *y;
    int len;
    pthread_mutex_t *lock;
    double local_result;
    double *global_result;
} dot_product_t;

void *perform_dot_product(void *args)
{
    dot_product_t *dp = (dot_product_t *)args;

    dp->local_result = 0.0;
    for (size_t i = 0; i < dp->len; i++)
    {
        dp->local_result += dp->x[i] * dp->y[i];
    }
    
    pthread_mutex_lock(dp->lock);
    *(dp->global_result) += dp->local_result;
    pthread_mutex_unlock(dp->lock);

    pthread_exit(0);
}

void fail_if_nonzero(int return_code)
{
    if (return_code != 0)
    {
        fprintf(stderr, "Failed with return code: %d", return_code);
        exit(-1);
    }
}

void print_vector(double *vec, size_t len)
{
    printf("[");
    for (size_t i = 0; i < len; i++)
    {
        if (i > 0)
        {
            printf(", ");
        }
        printf("%.3lf", vec[i]);
    }
    printf("]");
}

void print_vector_line(double *vec, size_t len)
{
    print_vector(vec, len);
    printf("\n");
}

int main(int argc, char **argv)
{
    dot_product_t dp[NTHREAD];
    pthread_mutex_t lock;
    pthread_t threads[NTHREAD];
    double x[N];
    double y[N];
    double result;

    pthread_mutex_init(&lock, NULL);
    srand(0);
    for (size_t i = 0; i < N; i++)
    {
        x[i] = rand() % 1000;
        y[i] = rand() % 1000;
    }

    result = 0.0;
    for (size_t i = 0; i < NTHREAD; i++)
    {
        dp[i].global_result = &result;
        dp[i].local_result = 0.0;
        dp[i].x = x + i * SEGLEN;
        dp[i].y = y + i * SEGLEN;
        dp[i].len = min(N - i * SEGLEN, SEGLEN);
        dp[i].lock = &lock;
    }

    for (size_t i = 0; i < NTHREAD; i++)
    {
        int return_code = pthread_create(threads + i, NULL, perform_dot_product, &dp[i]);
        fail_if_nonzero(return_code);
    }

    for (size_t i = 0; i < NTHREAD; i++)
    {
        int return_code = pthread_join(threads[i], NULL);
        fail_if_nonzero(return_code);
    }

    printf("sum = %.3f\n", result);

    pthread_mutex_destroy(&lock);
    return 0;
}
