/* filename: lab1b.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 2/20/15
 * brief: mat-mat multiplication with pthreads
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <unistd.h>

/* holds all per thread values */
struct worker_args
{
    int id;
    int size;
    int row_index;
    int num_rows;
    double *mat_a;
    double *mat_b;
    double *mat_c;
};

/* assumes one dimmensional array nxn mat format of type double */
void mat_print(double *mat, int size)
{
    int i, j;
    for (i = 0; i < size; i++)
    {   
        printf("[ ");
        for (j = 0; j < size; j++)
        {
            if (j != size - 1)
            {
                printf("%3.1f ", mat[i*size + j]);
            }
            else {
                printf("%3.1f ]\n", mat[i*size + j]);
            }
        }
    }
}

/* assumes one dimmensional array nxn mat format of type double */
void mat_init(double *mat, int size)
{
    int i, j;
    for (i = 0; i < size; i++)
    {
        for (j = 0; j < size; j++)
        {
            /* arbitrarily limited to 0 - 49 */
            mat[i*size + j] = (rand() % 50);
        }
    }
}

/* this function may be prone to false sharing 
 * shouldn't be any data contention though since 
 * each thread has it's own set of rows 
 */
void *mat_mult(void *arguments)
{
    int i, j, k, size, end_row;
    struct worker_args *args = (struct worker_args*) arguments;
    end_row = args->row_index + args->num_rows;
    size = args->size;
    
    for (i = args->row_index; i < end_row; i++)
    {
        for (j = 0; j < size; j++)
        {
            for (k = 0; k < size; k++)
            {
                args->mat_c[i*size + j] += \
                    args->mat_a[i*size + k] * args->mat_b[k*size + j];
            }
        }
    }

    /* this causes reachabled memory leaks due to pthread implementation
     * and issues related to unwinding the stack */
    //pthread_exit(NULL); 
    return NULL;
}


int main(int argc, char *argv[])
{
    if (3 > argc || 3 < argc)
    {
        printf("lab1b usage: ./lab1b n nthreads\n");
        exit(-1);
    }
    const int n = atoi(argv[1]);
    const int nthd = atoi(argv[2]);
    int rows_per_thd;
    int extra_rows;
    int rc, i;
    struct timeval st, et;
    struct timezone tz;
    long t_sec, t_usec;
    pthread_t threads[nthd];
    
    /* allocate matrices */
    double *mat_a = calloc(n*n, sizeof *mat_a);
    double *mat_b = calloc(n*n, sizeof *mat_b);
    double *mat_c = calloc(n*n, sizeof *mat_c);
    if (mat_a == NULL || mat_b == NULL || mat_c == NULL)
    {
        printf("Uh oh, a mat didn't get allocated. Exiting.\n");
        exit(1);
    }

    /* allocate array of struct pointers for args */
    struct worker_args **args = calloc(nthd, sizeof(struct worker_args*));

    /* allocate argument struct for each thread */
    for (i = 0; i < nthd; i++)
    {
        args[i] = calloc(1, sizeof(struct worker_args));
        if (args[i] == NULL)
        {
            printf("Uh oh, mat args didn't get allocated. Exiting.\n");
            exit(1);
        }
        /* initialize some values */
        args[i]->size = n;
        args[i]->mat_a = mat_a;
        args[i]->mat_b = mat_b;
        args[i]->mat_c = mat_c;
    }

    /* initialize matrices */
    srand(time(NULL));
    mat_init(mat_a, n);
    mat_init(mat_b, n);

    /* extra rows will be given to the last thread spawned */
    rows_per_thd = n / nthd;
    extra_rows = n % nthd;

    /* do actual work in the threads and time it */
    printf("Matrix dimmension: %d  Number of threads: %d\n", n, nthd);

    gettimeofday(&st, &tz);
    for(i = 0; i < nthd; i++)
    {
        args[i]->id = i;
        args[i]->row_index = i * rows_per_thd;
        args[i]->num_rows = rows_per_thd + ((i == nthd-1) ? extra_rows : 0);
        rc = pthread_create(&threads[i], NULL, mat_mult, (void *)args[i]);
        if (rc)
        {
            printf("ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    for(i = 0; i < nthd; i++)
    {
        rc = pthread_join(threads[i], NULL);
        free(args[i]);
        if (rc)
        {
            printf("ERROR: return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }
    gettimeofday(&et, &tz);
    t_sec = et.tv_sec - st.tv_sec;
    t_usec = (et.tv_usec - st.tv_usec) % 1000000ULL;
    printf("    Time elapsed: %lu.%lu sec\n", t_sec, t_usec);

    /* display results */
    if (n <= 40)
    {
        printf("Matrix A\n");
        mat_print(mat_a, n);
        printf("Matrix B\n");
        mat_print(mat_b, n);
        printf("Matrix C\n");
        mat_print(mat_c, n);
    } 
    else {
        printf ("    n > 40, matrix will not be printed\n");
    }

    /* cleanup */
    free(args);
    free(mat_a);
    free(mat_b);
    free(mat_c);
    
    return 0;
} 

