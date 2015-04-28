/* filename: final.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 4/29/15
 * brief: MPI SPMD - matrix mult using nb send/recv to overlap communication
 * and computation. Slave processes are multithreaded, utilizing a thread pool.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>
#include <pthread.h>
#include <assert.h>

#define MPI_MSTR 0x0
#define TASK_REQ 0xa5a5
#define SLAVE_END (int)0x5a5a

void *compute_row(void *args);
void spawn_threads(int *result, int *input, int *mat_b, int task_size, int n);
void master(int n, int np, int rank, int task_size, MPI_Comm comm);
void slave(int n, int np, int rank, int task_size, MPI_Comm comm);
void mat_init(int *mat, int size);
void mat_print(int *mat, int size);
void mat_mult(int *c, int *a, int *b, int n, int m);

struct worker_args {
    int n;
    int row;
    int *result;    /* chunk mat_c */
    int *buf;       /* chunk mat_a */
    int *mat_b;     /* all mat_b */
};

int main(int argc, char *argv[])
{
    int rank, n, np;
    int task_size;
    MPI_Comm comm = MPI_COMM_WORLD;

    if (argc != 3) {
        printf("usage: final [matrix_size] [task_size]\n");
        exit(-1);
    }

    n = atoi(argv[1]);
    task_size = atoi(argv[2]);
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &np); 


    if (rank == MPI_MSTR) {
        master(n, np, rank, task_size, comm);
    } else {
        slave(n, np, rank, task_size, comm);
    }

    MPI_Finalize();
    return 0;
}

/**************************************************
 * Multithreaded functions
 **************************************************/
void *compute_row(void *args)
{
    int chunk;
    struct worker_args *arg = (struct worker_args*) args;

    chunk = arg->row * arg->n;
    mat_mult(&arg->result[chunk], &arg->buf[chunk], arg->mat_b, arg->n, 1);
    return NULL;
}

void spawn_threads(int *result, int *input, int *mat_b, int task_size, int n) 
{
        int i;
        pthread_t threads[task_size];

        struct worker_args **args;
        assert((args = malloc(task_size * sizeof(struct worker_args*))) != NULL);
        memset(result, 0, n * task_size * sizeof *result);
        for (i = 0; i < task_size; i++) {
            assert((args[i] = malloc(sizeof(struct worker_args))) != NULL);
            args[i]->n = n;
            args[i]->row = i;
            args[i]->result = result;
            args[i]->buf = input;
            args[i]->mat_b = mat_b;
            assert((pthread_create(&threads[i], NULL, compute_row, (void *)args[i])) == 0);
        }

        for (i = 0; i < task_size; i++) {
            assert((pthread_join(threads[i], NULL)) == 0);
            free(args[i]);
        }

        free(args);
}

/**************************************************
 * MPI functions 
 **************************************************/
/* MPI master - creates and distributes mat_a, mat_b. Combines results from slaves */ 
void master(int n, int np, int rank, int task_size, MPI_Comm comm)
{
    int i;
    int cnt;
    int chunk;
    int worker;
    int *mat_a, *mat_b, *mat_c;
    int *task, *rbuf;
    double t1, t2;
    MPI_Status stat;
    
    chunk = n * task_size;

    assert((rbuf = malloc(chunk * sizeof *rbuf)) != NULL);
    assert((mat_a = malloc(n*n * sizeof *mat_a)) != NULL);
    assert((mat_b = malloc(n*n * sizeof *mat_b)) != NULL);
    assert((mat_c = calloc(n*n,  sizeof *mat_c)) != NULL);

    printf("n: %d npes: %d task_size: %d\n", n, np, task_size);

    srand(time(NULL));
    mat_init(mat_a, n);
    mat_init(mat_b, n);

    t1 = MPI_Wtime();

    MPI_Bcast(mat_b, n*n, MPI_INT, rank, comm);
    /* distribute tasks to worker pool */
    i = 0;
    while (i < n) {
        MPI_Recv(&worker, 1, MPI_INT, MPI_ANY_SOURCE, TASK_REQ, comm, &stat);
        task = &mat_a[i*n];
        MPI_Send(task, chunk, MPI_INT, worker, i*n, comm);
        i += task_size;
    }

    /* terminate outstanding task requests */
    for (i = 1; i < np; i++) {
        MPI_Recv(&worker, 1, MPI_INT, MPI_ANY_SOURCE, TASK_REQ, comm, &stat);
        MPI_Send(task, 1, MPI_INT, worker, SLAVE_END, comm);
    }

    /* combine partial results */
    i = 0;
    while (i < n) {
        MPI_Recv(rbuf, chunk, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &stat);
        MPI_Get_count(&stat, MPI_INT, &cnt);
        memcpy(&mat_c[stat.MPI_TAG], rbuf, cnt * sizeof(cnt));
        i += task_size;
    }

    t2 = MPI_Wtime();
    printf("    Elapsed time is %f sec\n", t2 - t1);

    if (n <= 16) {
        printf("    matrix a\n");
        mat_print(mat_a, n);
        printf("    matrix b\n");
        mat_print(mat_b, n);
        printf("    matrix c\n");
        mat_print(mat_c, n);
    } else {
        printf("    n > 16, will not print matrices\n");
    }

    free(rbuf);
    free(mat_a);
    free(mat_b);
    free(mat_c);
}

/* MPI slave - gets available tasks and computes part of mat_c */
void slave(int n, int np, int rank, int task_size, MPI_Comm comm)
{
    int i, j; 
    int cnt, chunk;
    int *mat_b, *buf_a, *buf_b, *result_a, *result_b;
    MPI_Status stat_a, stat_b;
    MPI_Request req_a, req_b, req_dummy;

    chunk = n * task_size;

    assert((result_a = malloc(chunk * sizeof *result_a)) != NULL);
    assert((result_b = malloc(chunk * sizeof *result_b)) != NULL);
    assert((buf_a = malloc(chunk * sizeof *buf_a)) != NULL);
    assert((buf_b = malloc(chunk * sizeof *buf_b)) != NULL);
    assert((mat_b = malloc(n*n * sizeof *mat_b)) != NULL);

    /* receive mat_b */
    MPI_Bcast(mat_b, n*n, MPI_INT, MPI_MSTR, comm);

    /* alternate buffers to overlap comm and comp */
    i = 0;
    while (1) {
        /* buf_a comm, buf_b comp */
        MPI_Isend(&rank, 1, MPI_INT, MPI_MSTR, TASK_REQ, comm, &req_dummy);
        MPI_Irecv(buf_a, chunk, MPI_INT, MPI_MSTR, MPI_ANY_TAG, comm, &req_a);

        if (i != 0) {
            spawn_threads(result_b, buf_b, mat_b, task_size, n);
            MPI_Isend(result_b, chunk, MPI_INT, MPI_MSTR, stat_b.MPI_TAG, comm, &req_dummy);
        }

        MPI_Wait(&req_a, &stat_a);
        if (stat_a.MPI_TAG == SLAVE_END) {
            break;
        }

        /* buf_b comm, buf_a comp */
        MPI_Isend(&rank, 1, MPI_INT, MPI_MSTR, TASK_REQ, comm, &req_dummy);
        MPI_Irecv(buf_b, chunk, MPI_INT, MPI_MSTR, MPI_ANY_TAG, comm, &req_b);

        spawn_threads(result_a, buf_a, mat_b, task_size, n);
        MPI_Isend(result_a, chunk, MPI_INT, MPI_MSTR, stat_a.MPI_TAG, comm, &req_dummy);

        MPI_Wait(&req_b, &stat_b);
        if (stat_b.MPI_TAG == SLAVE_END) {
            break;
        }
        i++;
    }

    free(result_a);
    free(result_b);
    free(mat_b);
    free(buf_a);
    free(buf_b);
}

/**************************************************
 * Matrix functions 
 *    assume one dimmensional nxn int matrices
 **************************************************/
/* prints nxn matrix */
void mat_print(int *mat, int size)
{
    int i, j;
    for (i = 0; i < size; i++) {   
        printf("    [ ");
        for (j = 0; j < size; j++) {
            if (j != size - 1) {
                printf("%d ", mat[i*size + j]);
            }
            else {
                printf("%d ]\n", mat[i*size + j]);
            }
        }
    }
}

/* initializes nxn matrix with ints 0-49 */
void mat_init(int *mat, int size)
{
    int i, j;
    for (i = 0; i < size; i++) {
        for (j = 0; j < size; j++) {
            mat[i*size + j] = rand() % 50;
        }
    }
}

/* multiplies mxn and nxn matrices */
void mat_mult(int *c, int *a, int *b, int n, int m)
{
    int i, j, k;
    for (i = 0; i < m; i++) {
        for (j = 0; j < n; j++) {
            c[i*n + j] = 0;
            for (k = 0; k < n; k++) {
                c[i*n + j] += a[i*n + k] * b[k*n + j];
            }
        }
    }
}

