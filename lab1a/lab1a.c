/* filename: lab1a.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 2/13/15
 * brief: first pthreads program
 */

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#define NUM_THD 5

void *PrintHello(void *threadid)
{
    printf("\nHello from thread %d\n", threadid);
    pthread_exit(NULL);
}

int main(int argc, char *argv[])
{
    pthread_t threads[NUM_THD];
    int rc, t;
    for(t=0; t<NUM_THD; t++){
    rc = pthread_create(&threads[t], NULL, PrintHello, (void *)t);
    if (rc){
        printf("ERROR: return error from pthread_create() is %d\n", rc);
        exit(-1);
    }
}
    for(t=0; t<NUM_THD; t++){
        rc = pthread_join(threads[t], NULL);
        if (rc){
            printf("ERROR: return error from pthread_join() is %d\n", rc);
            exit(-1);
        }
    }
    return 0;
} 
