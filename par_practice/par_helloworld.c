#include <omp.h>
#include <stdio.h>

int main()
{
    for (int i = 0; i < 1; i++) 
    {
        #pragma omp parallel
        {
            printf("omp_get_num_threads = %d\n", omp_get_num_threads());
            printf("omp_get_thread_num = %d\n", omp_get_thread_num());
            printf("omp_get_num_procs = %d\n", omp_get_num_procs());
            printf("%d\n", i);
        }
    }
    return 0;
}
