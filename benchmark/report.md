## Solution Explanation

In all implementation, we employ cache-friendly single dimensional arrays of size N * N to represent square matrices. 

This has some advantages:
1. All elements are in one continueous block of memory, for example A[1 .. N * N]
2. Only one memory access to reach the element, e.g., A[i * N + j] instead of A[i][j]
3. Adjacent elements for both row-wise and column-wise access offer good spatial locality

### Naive non-blocking implementation

The function is `void matrix_mult_naive(matrix_t *, matrix_t *, matrix_t *, double *)`.

This function uses three nested loop to multiply matrix A by matrix B. In this naive code, we don't apply optimizations, i.e., loop tiling, temperary variables, or parallel computation, etc.

```c
for (int i = 0; i < N; i++)
{
    for (int j = 0; j < N; j++)
    {
        C->mem[i * N + j] = 0.0;
        for (int k = 0; k < N; k++)
        {
            C->mem[i * N + j] += A->mem[i * N + k] * B->mem[k * N + j];
        }
    }
}
```

### Blocked matrix implementation

The function is `void matrix_mult_block(matrix_t *, matrix_t *, int, matrix_t *, double *)`.

The implementation uses two optimizations:
1. Use temparary variable `sum` to reduce pointer access to C[i][j]
2. Divide the matrix into `N / block_size` square sub-matrices (called blocks). The three outer loops (`bi`, `bj`, and `bk`) browse through each block. 
    - Loops `bi` and `bj` select the result block in matrix C
    - Loop `bk` selects blocks in A and B to compute for this block in C
3. Three inner loops `i`, `j`, and `k` 

```c
for (int bi = 0; bi < N; bi += block_size)
{
    for (int bj = 0; bj < N; bj += block_size)
    {
        for (int bk = 0; bk < N; bk += block_size)
        {
            int i_end = min(bi + block_size, N);
            int j_end = min(bj + block_size, N);
            int k_end = min(bk + block_size, N);
            
            for (int i = bi; i < i_end; i++)
            {
                for (int j = bj; j < j_end; j++)
                {
                    double sum = 0; // temparary variable to store summation
                    for (int k = bk; k < k_end; k++)
                    {
                        sum += A->mem[i * N + k] * B->mem[k * N + j]; // loop tiling
                    }
                    C->mem[i * N + j] = sum;
                }
            }
        }
    }
}
```