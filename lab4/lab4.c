/* filename: lab4.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 3/20/15
 * brief: MPI SPMD - matrix mult
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>

void master(int n, int np, int *sbuf, int *rbuf);
void slave(int rank, int n, int np, int *sbuf, int *rbuf_a, int *rbuf_b);
void mat_print(int *mat, int size);
void mat_init(int *mat, int size);
void mat_mult(int *c, int *a, int *b, int n, int row_num);

int main(int argc, char *argv[])
{
    int rank, n, np;
    int *sbuf, *rbuf_a, *rbuf_b;
    struct timeval st, et;
    long t_sec, t_usec;

    if (argc != 2) {
        printf("usage: lab4 [n] where n is matrix size\n");
        exit(-1);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np); 
    n = atoi(argv[1]);

    sbuf = calloc(n*n, sizeof *sbuf);
    rbuf_a = calloc(n*n, sizeof *rbuf_a);
    rbuf_b = calloc(n*n, sizeof *rbuf_b);
    if (sbuf == NULL || rbuf_a == NULL || rbuf_b == NULL) {
        printf("Error allocating buffers. Exiting\n");
        exit(-1);
    }

    if (rank == 0) {
        gettimeofday(&st, NULL);
        master(n, np, sbuf, rbuf_a);
        gettimeofday(&et, NULL);
        t_sec = et.tv_sec - st.tv_sec;
        t_usec = (et.tv_usec - st.tv_usec) % 1000000ULL;
        printf("    Time elapsed: %lu.%lu sec\n", t_sec, t_usec);
    } else {
        slave(rank, n, np, sbuf, rbuf_a, rbuf_b);
    }

    free(sbuf);
    free(rbuf_a);
    free(rbuf_b);

    MPI_Finalize();
    return 0;
}

/**************************************************
 * MPI worker functions 
 **************************************************/
/* MPI master - creates and distributes mat_a and mat_b. Combines results from slaves */
void master(int n, int np, int *sbuf, int *rbuf)
{
    int i;
    int row_chunk, row_extra, row_num;
    int *mat_a, *mat_b, *mat_c;
    MPI_Status status;

    mat_a = calloc(n*n, sizeof *mat_a);
    mat_b = calloc(n*n, sizeof *mat_b);
    mat_c = calloc(n*n, sizeof *mat_c);
    if (mat_a == NULL || mat_b == NULL || mat_c == NULL) {
        printf("Error allocating matrices. Exiting\n");
        exit(-1);
    }

    srand(time(NULL));
    mat_init(mat_a, n);
    mat_init(mat_b, n);

    row_chunk = n / (np - 1);
    row_extra = n % (np - 1);
    
    printf("n: %d npes: %d \n", n, np);
    
    /* distribute chunk mat_a and all mat_b */
    for (i = 1; i < np; i++) {
        row_num = row_chunk + ((i == (np - 1)) ? row_extra : 0);

        memcpy(sbuf, &mat_a[(i-1)*n*row_chunk], n*row_num*sizeof *mat_a);
        MPI_Send(sbuf, n*row_num, MPI_INT, i, 1, MPI_COMM_WORLD);

        memcpy(sbuf, mat_b, n*n*sizeof *mat_b);
        MPI_Send(sbuf, n*n, MPI_INT, i, 2, MPI_COMM_WORLD);
    }

    /* combine partial results */
    for (i = 1; i < np; i++) {
        row_num = row_chunk + ((i == (np - 1)) ? row_extra : 0);
        MPI_Recv(rbuf, n*row_num, MPI_INT, i, 3, MPI_COMM_WORLD, &status);
        memcpy(&mat_c[(i-1)*n*row_chunk], rbuf, n*row_num*sizeof *mat_c);
    }

    if (n <= 16) {
        printf("matrix a\n");
        mat_print(mat_a, n);
        printf("matrix b\n");
        mat_print(mat_b, n);
        printf("matrix c\n");
        mat_print(mat_c, n);
    } else {
        printf("n > 16, will not print matrices\n");
    }

    free(mat_a);
    free(mat_b);
    free(mat_c);
}

/* MPI slave - computes portion of matrix multiplication */
void slave(int rank, int n, int np, int *sbuf, int *rbuf_a, int *rbuf_b)
{
    int row_chunk, row_extra, row_num;
    MPI_Status status;

    row_chunk = n / (np - 1);
    row_extra = n % (np - 1);
    row_num = row_chunk + ((rank == (np - 1)) ? row_extra : 0);

    /* receive chunk mat_a and all mat_b */
    MPI_Recv(rbuf_a, n*row_num, MPI_INT, 0, 1, MPI_COMM_WORLD, &status);
    MPI_Recv(rbuf_b, n*n,       MPI_INT, 0, 2, MPI_COMM_WORLD, &status);

    /* compute portion of mat_c, send to master */
    mat_mult(sbuf, rbuf_a, rbuf_b, n, row_num);
    MPI_Send(sbuf, n*row_num, MPI_INT, 0, 3, MPI_COMM_WORLD);
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
        printf("[ ");
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
            for (k = 0; k < n; k++) {
                c[i*n + j] += a[i*n + k] * b[k*n + j];
            }
        }
    }
}

