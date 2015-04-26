/*
 *Example - Hello world
 */

#include <stdio.h>
#include <mpi.h>

int main(int argc, char *argv[])
{
    int namelen;
    int rank, size;
    char processor_name[MPI_MAX_PROCESSOR_NAME];

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Get_processor_name(processor_name,&namelen);
    printf("Hello world! I am %d of %d on %s\n", rank, size, processor_name);
    MPI_Finalize();
    return 0;
}

