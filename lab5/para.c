/* filename: para.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 3/27/15
 * brief: MPI odd-even sort
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <mpi.h>

void *arr_init(int *array, int size);
void arr_print(int *array, int size);
void compare_split(int nlocal, int *elmnts, int *relmnts, int *wspace, int keepsmall);
int MPI_OddEvenSort(int *arr, int n, int np, int rank, int root, \
                    MPI_Comm comm, int sort_flag);

int main(int argc, char *argv[])
{
    int n, np;
    int rank, root;
    int sort_flag;
    int *arr;
    MPI_Comm comm = MPI_COMM_WORLD;

    if (argc != 3) {
        printf("Usage: lab5 [n] [flag] \n    where n is num elements and flag is sorting alg\n");
        exit(-1);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &np); 

    n = atoi(argv[1]);
    sort_flag = atoi(argv[2]);
    root = 0;

    if (rank == root) {
        printf("n: %d np: %d sort: %d\n", n, np, sort_flag);
        arr = calloc(n, sizeof *arr);
        if (arr == NULL) {
            printf("Unable to allocate arrays. Exiting\n");
            exit(1);
        }
        srand(time(NULL));
        arr_init(arr, n);
        if (n <= 40) {
            printf("Unsorted array:\n");
            arr_print(arr, n);
        }
    }

    MPI_OddEvenSort(arr, n, np, rank, root, comm, sort_flag);

    if (rank == root) {
        if (n <= 40) {
            printf("Sorted array:\n");
            arr_print(arr, n);
        }
        free(arr);
    }

    MPI_Finalize();
    return 0;
}

int MPI_OddEvenSort(int *arr, int n, int np, int rank, int root, \
                    MPI_Comm comm, int sort_flag)
{
    int i;
    int nlocal;
    int evenrank, oddrank;
    int *alocal, *wspace, *relmnts;
    double t1, t2;
    MPI_Status status;

    nlocal = n / np;
    alocal  = malloc(nlocal * sizeof *alocal);
    wspace  = malloc(nlocal * sizeof *wspace);
    relmnts = malloc(nlocal * sizeof *relmnts);
    if (alocal == NULL || wspace == NULL || relmnts == NULL) {
        printf("Error allocating odd-even sort buffers. Exiting\n");
        exit(1);
    }

    /* determine communication partners */
    if (rank % 2 == 0) {
        oddrank  = rank + 1;
        evenrank = rank - 1;
    } else {
        oddrank  = rank - 1;
        evenrank = rank + 1;
    }
    if (oddrank == -1 || oddrank == np) {
        oddrank = MPI_PROC_NULL;
    }
    if (evenrank == -1 || evenrank == np) {
        evenrank = MPI_PROC_NULL;
    }

    /* begin */
    if (rank == 0) {
        t1 = MPI_Wtime();
    }
    MPI_Scatter(arr, nlocal, MPI_INT, alocal, nlocal, MPI_INT, root, comm);

    if (sort_flag == 0) {
        wquicksort(alocal, 0, nlocal-1, 1);
    } else {
        wbubblesort(alocal, nlocal);
    }

    /* odd-even sort */
    for (i = 1; i <= np; i++) {
        if (i%2 == 1) {
            MPI_Sendrecv(alocal, nlocal, MPI_INT, oddrank, 1, \
                        relmnts, nlocal, MPI_INT, oddrank, 1, comm, &status);
            compare_split(nlocal, alocal, relmnts, wspace, rank < status.MPI_SOURCE);
        } else {
            MPI_Sendrecv(alocal, nlocal, MPI_INT, evenrank, 1, \
                        relmnts, nlocal, MPI_INT, evenrank, 1, comm, &status);
            if (rank !=0 && rank != np-1) {
                compare_split(nlocal, alocal, relmnts, wspace, rank < status.MPI_SOURCE);
            }
        }
    }

    MPI_Gather(alocal, nlocal, MPI_INT, arr, nlocal, MPI_INT, root, comm);

    if (rank == 0) {
        t2 = MPI_Wtime();
        printf("Elapsed time is %f sec\n", t2 - t1);
    }
    /* end */

    free(alocal); 
    free(wspace);
    free(relmnts);
    return MPI_SUCCESS;
}

/* nlocal : number of elements to be stored locally = n / nthd
 * elmnts : array which stores local elements
 * relmnts : array which stores remote elements
 * wspace : working space during swaps 
 * keepsmall : keep smaller half if set, larger otherwise
 */ 
void compare_split(int nlocal, int *elmnts, int *relmnts, int *wspace, int keepsmall)
{
    int i, j, k;

    /* copy elements into working space */
    for(i=0; i<nlocal; i++) {
        wspace[i] = elmnts[i];
    }

    /* keep nlocal smaller elements */
    if (keepsmall) {
        for(i=j=k=0; k<nlocal; k++) {
            if(j == nlocal || (i < nlocal && wspace[i] < relmnts[j])) {
                elmnts[k] = wspace[i++];
            } else {
                elmnts[k] = relmnts[j++];
            }
        }
    /* keep nlocal larger elements */
    } else { 
        for(i=k=nlocal-1, j=nlocal-1; k>=0; k--) {
            if(j==-1 || (i >= 0 && wspace[i] >= relmnts[j])) {
                elmnts[k] = wspace[i--];
            } else {
                elmnts[k] = relmnts[j--];
            }
        }
    }
}
 
/**************************************************
 * Array functions 
 **************************************************/
/* initializes int array with random ints */
void *arr_init(int *array, int size)
{
    int i;
    for (i = 0; i < size; i++) {
        array[i] = rand();
    }
}

/* prints integer array up to size */
void arr_print(int *array, int size)
{
    int i;
    printf("[ ");
    for (i = 0; i < size; i++) {
        printf("%d ", array[i]);
    }
    printf("]\n");
}

