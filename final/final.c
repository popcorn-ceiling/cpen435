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
#include "./threadpool/src/threadpool.h"

#define MPI_MSTR 0x0
#define TASK_REQ 0xa5a5
#define SLAVE_END (int)0x5a5a
#define ROW_DONE 0x1

void compute_row(void *args);
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
    int *row_done;
};

int main(int argc, char *argv[])
{
    int rank, n, np;
    int task_size;
    MPI_Comm comm = MPI_COMM_WORLD;

    if (argc != 2) {
        printf("usage: final [n] where n is matrix size\n");
        exit(-1);
    }

    n = atoi(argv[1]);
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &np); 

    task_size = 4;

    if (rank == MPI_MSTR) {
        master(n, np, rank, task_size, comm);
    } else {
        slave(n, np, rank, task_size, comm);
    }

    MPI_Finalize();
    return 0;
}

/**************************************************
 * MPI worker functions 
 **************************************************/
/* Multithreaded functioned, used by thread pool */
void compute_row(void *args)
{
    struct worker_args *arg = (struct worker_args*) args;

    printf("about to mult row %d!!!\n", arg->row);
    mat_mult(&arg->result[arg->row], &arg->buf[arg->row*arg->n], arg->mat_b, arg->n, 1);
    printf("about to update row %d!!!\n", arg->row);
    arg->row_done[arg->row] = ROW_DONE;
}

/* MPI master - creates and distributes mat_a, mat_b. Combines results from slaves */ 
void master(int n, int np, int rank, int task_size, MPI_Comm comm)
{
    int i;
    int cnt;
    int worker;
    int *mat_a, *mat_b, *mat_c;
    int *task, *rbuf;
    double t1, t2;
    MPI_Status stat;

    printf("n: %d npes: %d \n", n, np);
    
    assert((rbuf = malloc(n * task_size * sizeof *rbuf)) != NULL);
    assert((mat_a = malloc(n*n * sizeof *mat_a)) != NULL);
    assert((mat_b = malloc(n*n * sizeof *mat_b)) != NULL);
    assert((mat_c = calloc(n*n,  sizeof *mat_c)) != NULL);

    srand(time(NULL));
    mat_init(mat_a, n);
    mat_init(mat_b, n);

    t1 = MPI_Wtime();

    MPI_Bcast(mat_b, n*n, MPI_INT, rank, comm);
    printf("m1\n");
    /* distribute tasks to worker pool */
    i = 0;
    while (i < n) {
        MPI_Recv(&worker, 1, MPI_INT, MPI_ANY_SOURCE, TASK_REQ, comm, &stat);
        task = &mat_a[i*n*task_size];
        MPI_Send(task, n*task_size, MPI_INT, worker, i, comm);
        i++;
    }

    printf("m2\n");
    /* terminate outstanding task requests */
    for (i = 1; i < np; i++) {
        MPI_Recv(&worker, 1, MPI_INT, MPI_ANY_SOURCE, TASK_REQ, comm, &stat);
        MPI_Send(task, 1, MPI_INT, worker, SLAVE_END, comm);
    }

    printf("m3\n");
    /* combine partial results */
    i = 0;
    while (i < n) {
        MPI_Recv(rbuf, n*task_size, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &stat);
        MPI_Get_count(&stat, MPI_INT, &cnt);
        memcpy(&mat_c[(stat.MPI_TAG)*n*task_size], rbuf, cnt * sizeof(cnt));
        i++;
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
    int i, j, all_done, cnt;
    int *mat_b, *buf_a, *buf_b, *result, *row_done;
    struct worker_args **args;
    threadpool_t *pool;
    MPI_Status stat_a, stat_b;
    MPI_Request req_a, req_b, req_dummy;

    assert((result = malloc(n * task_size * sizeof *result)) != NULL);
    assert((mat_b = malloc(n*n * sizeof *mat_b)) != NULL);
    assert((buf_a = malloc(n * sizeof *buf_a)) != NULL);
    assert((buf_b = malloc(n * sizeof *buf_b)) != NULL);
    assert((row_done = malloc(n * task_size * sizeof(int))) != NULL);
    assert((args = malloc(sizeof(struct worker_args*))) != NULL);
    for (i = 0; i < task_size; i++) {
        assert((args[i] = malloc(sizeof(struct worker_args))) != NULL);
        args[i]->n = n;
        args[i]->row_done = row_done;
        args[i]->result = result;
        args[i]->mat_b = mat_b;
    }

    /* initialize thread pool */
    assert((pool = threadpool_create(task_size, task_size, 0)) != NULL);

    /* receive mat_b */
    MPI_Bcast(mat_b, n*n, MPI_INT, MPI_MSTR, comm);

    /* alternate buffers to overlap comm and comp */
    i = 0;
    while (1) {
        printf("s1 %d\n", rank);
        /* buf_a comm, buf_b comp */
        MPI_Isend(&rank, 1, MPI_INT, MPI_MSTR, TASK_REQ, comm, &req_dummy);
        MPI_Irecv(buf_a, n*task_size, MPI_INT, MPI_MSTR, MPI_ANY_TAG, comm, &req_a);

        printf("s2 %d\n", rank);
        if (i !=0) {
            /* use thread pool, task is computing one row */
            for (j = 0; j < task_size; j++) {
                args[j]->buf = buf_b;
                args[j]->row = j;
                threadpool_add(pool, &compute_row, args[j], 0);
            }
            printf("s3 %d\n", rank);
            /* wait for all rows to finish */
            all_done = 0;
            while (all_done != 1) {
                all_done = 1; /* initial condition */
                for (j = 0; j < task_size; j++) {
                    all_done &= args[0]->row_done[j]; /* will be 0 if any of the result aren't done */
                }
            }
            printf("s4 %d\n", rank);
            MPI_Isend(result, n*task_size, MPI_INT, MPI_MSTR, stat_b.MPI_TAG, comm, &req_dummy);
        }

        printf("s5 %d\n", rank);
        memset(result, 0, n * task_size * sizeof *result);
        memset(args[0]->row_done, 0, n * task_size * sizeof(int));
        MPI_Wait(&req_a, &stat_a);
        if (stat_a.MPI_TAG == SLAVE_END) {
            break;
        }

        printf("s6 %d\n", rank);
        /* buf_b comm, buf_a comp */
        MPI_Isend(&rank, 1, MPI_INT, MPI_MSTR, TASK_REQ, comm, &req_dummy);
        MPI_Irecv(buf_b, n*task_size, MPI_INT, MPI_MSTR, MPI_ANY_TAG, comm, &req_b);

        printf("s7 %d\n", rank);
        /* use thread pool, task is computing one row */
        for (j = 0; j < task_size; j++) {
            args[j]->buf = buf_a;
            args[j]->row = j;
            threadpool_add(pool, &compute_row, args[j], 0);
        }
        printf("s8 %d\n", rank);
        /* wait for all rows to finish */
        all_done = 0;
        while (all_done != 1) {
            all_done = 1; /* initial condition */
            for (j = 0; j < task_size; j++) {
                all_done &= args[0]->row_done[j]; /* will be 0 if any of the result aren't done */
            }
        }
        mat_print(result, task_size);
        
        printf("s9 %d\n", rank);
        MPI_Isend(result, n*task_size, MPI_INT, MPI_MSTR, stat_a.MPI_TAG, comm, &req_dummy);
        memset(result, 0, n * task_size * sizeof *result);
        memset(args[0]->row_done, 0, n * task_size * sizeof(int));
        MPI_Wait(&req_b, &stat_b);
        if (stat_b.MPI_TAG == SLAVE_END) {
            break;
        }
        i++;
        printf("s10 %d\n", rank);
    }

    printf("slave %d done with loop\n", rank);
    assert(threadpool_destroy(pool, 0) == 0);
    for (i = 0; i < task_size; i++) {
        free(args[i]);
    }
    free(args);
    free(result);
    free(row_done);
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
            c[i*n +j] = 0;
            for (k = 0; k < n; k++) {
                c[i*n + j] += a[i*n + k] * b[k*n + j];
            }
        }
    }
}

