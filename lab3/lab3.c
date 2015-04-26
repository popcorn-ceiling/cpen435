/* filename: lab3.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 3/6/15
 * brief: MPI SPMD - exchange PIDs and report the min from each process
 */

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int i;
    int rank, np;
    int prev, next;
    int min_pid, min_rank;
    int sbuf[2], rbuf[2];
    MPI_Status status;
    pid_t pid;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &np); 

    pid = getpid();
    min_pid = pid;
    min_rank = rank;
    sbuf[0] = pid;
    sbuf[1] = rank;

    /* aaand trade places! */
    for (i = 1; i < np; i++) {
        next = (rank + i) % np;
        prev = (rank - i + np) % np;
        /* have the higher rank process send first */
        if (rank > next) {
            MPI_Send(sbuf, 2, MPI_INT, next, 1, MPI_COMM_WORLD);
            MPI_Recv(rbuf, 2, MPI_INT, prev, 1, MPI_COMM_WORLD, &status);
        } else {
            MPI_Recv(rbuf, 2, MPI_INT, prev, 1, MPI_COMM_WORLD, &status);
            MPI_Send(sbuf, 2, MPI_INT, next, 1, MPI_COMM_WORLD);
        }

        /* find min pid and associated process */
        if (rbuf[0] < min_pid) {
            min_pid = rbuf[0];
            min_rank = rbuf[1];
        }
    }

    printf("Process %d - ID: %d ; smallest process ID: %d from Process %d\n", \
            rank, pid, min_pid, min_rank);

    MPI_Finalize();
    return 0;
}

