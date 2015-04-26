/* filename: sort.c
 * author: daniel collins (dcollins3@zagmail.gonzaga.edu)
 * date: 2/27/15
 * brief: pthreads odd-even sort
 */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>
#include <string.h>

/* STRUCTURES */
struct my_barrier_t {
    pthread_mutex_t count_lock;
    pthread_cond_t proceed;
    int count;
};    

/* it is probably poor design to have so many arguments */
struct worker_args {
    int id;
    int nlocal;
    int nthread;
    int *arr;
    struct my_barrier_t *b_phase;
    struct my_barrier_t *b_buf;
};

/* FUNCTION PROTOTYPES */
extern void quicksort(int *a, int n);

int *arr_init(int size);
void arr_print(int *array, int size);
struct worker_args **args_init(int *array, int size, int num_thd, \
                               struct my_barrier_t *bp, struct my_barrier_t *bb);
void my_barrier_init(struct my_barrier_t *b);
void my_barrier(struct my_barrier_t *b, int num_thd);
void diff_time(struct timeval *st, struct timeval *et, struct timeval *dt);
void compare_split(int nlocal, int *elmnts, int *relmnts, int *wspace, int keepsmall);
void *p_odd_even_sort(void *args);

int main(int argc, char *argv[])
{
    if (3 != argc) {
        printf("lab2 usage: ./lab2 n nthreads\n");
        exit(-1);
    }

    int i;
    int rc;
    const int n = atoi(argv[1]);
    const int nthd = atoi(argv[2]);
    pthread_t threads[nthd];

    struct timeval st, et, dt;

    int *arr;
    int *arr_u;
    struct worker_args **args;
    struct my_barrier_t *bp = calloc(1, sizeof(struct my_barrier_t));
    struct my_barrier_t *bb = calloc(1, sizeof(struct my_barrier_t));
    
    printf("Array size: %d  Number of threads: %d\n", n, nthd); 
    if (n % nthd != 0) {
        printf("you goof'd, array size is not perfectly divisible by thread count\n");
        exit(-1);
    }

    srand(time(NULL));
    arr = arr_init(n);
    my_barrier_init(bb); 
    my_barrier_init(bp); 
    args = args_init(arr, n, nthd, bp, bb);

    /* keep unsorted array for posterity */
    arr_u = malloc(n * sizeof *arr);
    memcpy(arr_u, arr, n * sizeof *arr);

    gettimeofday(&st, NULL);
    for(i = 0; i < nthd; i++) {
        rc = pthread_create(&threads[i], NULL, p_odd_even_sort, (void *)args[i]);
        if (rc) {
            printf("ERROR: return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }

    /* cleanup threads and args */
    for(i = 0; i < nthd; i++) {
        rc = pthread_join(threads[i], NULL);
        if (rc) {
            printf("ERROR: return code from pthread_join() is %d\n", rc);
            exit(-1);
        }
        free(args[i]);
    }
    gettimeofday(&et, NULL);

    diff_time(&st, &et, &dt);
    printf("    Time elapsed: %lu.%lu sec\n", dt.tv_sec, dt.tv_usec);

    /* display results */
    if (n <= 40) {
        printf("Array unsorted:\n");
        arr_print(arr_u, n);
        printf("Array sorted:\n");
        arr_print(arr, n);
    } else {
        printf ("    n > 40, array will not be printed\n");
    }

    /* cleanup everything else */
    free(args);
    free(bp);
    free(bb);
    free(arr);
    free(arr_u);
    
    return 0;
} 

/* allocates and initializes int array with random ints between 0 and 99 */
int *arr_init(int size)
{
    int i;

    int *array = calloc(size, sizeof *array);
    if (array == NULL) {
        printf("Unable to allocate array. Exiting.\n");
        exit(1);
    }

    for (i = 0; i < size; i++) {
        array[i] = rand() % 100;
    }

    return array;
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

struct worker_args **args_init(int *array, int size, int num_thd, \
                               struct my_barrier_t *bp, struct my_barrier_t *bb)
{
    int thd;

    /* allocate array of struct pointers for args */
    struct worker_args **args = calloc(num_thd, sizeof(struct worker_args*));
    if (args == NULL) {
        printf("Unable to allocate args container. Exiting.\n");
        exit(1);
    }

    /* allocate argument struct for each thread */
    for (thd = 0; thd < num_thd; thd++) {
        args[thd] = calloc(1, sizeof(struct worker_args));
        if (args[thd] == NULL) {
            printf("Unable to allocate arg struct for thd %d. Exiting.\n", thd);
            exit(1);
        }

        /* initialize values */
        args[thd]->id = thd;
        args[thd]->nlocal = size / num_thd;
        args[thd]->nthread = num_thd;
        args[thd]->arr = array;
        args[thd]->b_phase = bp;
        args[thd]->b_buf = bb;
    }

    return args;
}

void my_barrier_init(struct my_barrier_t *b)
{
    b->count = 0;
    pthread_mutex_init(&b->count_lock, NULL);
    pthread_cond_init(&b->proceed, NULL);
}

/* barrier function to sync thread phases */
void my_barrier(struct my_barrier_t *b, int num_thd)
{
    pthread_mutex_lock(&b->count_lock);
    b->count++;
    if (b->count == num_thd) {
        b->count = 0;
        pthread_cond_broadcast(&b->proceed);
    } else {
        while (pthread_cond_wait(&b->proceed, &b->count_lock) != 0);
    }
    pthread_mutex_unlock(&b->count_lock);
}

void diff_time(struct timeval *st, struct timeval *et, struct timeval *dt)
{
    dt->tv_sec = et->tv_sec - st->tv_sec;
    dt->tv_usec = (et->tv_usec - st->tv_usec) % 1000000ULL;
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

/* multithreaded odd even sort */
void *p_odd_even_sort(void *arguments)
{
    int i;
    int ks;
    int skip;
    struct worker_args *args = (struct worker_args *)arguments;
    int *alocal = &args->arr[args->id * args->nlocal];
    int chunk_bytes = args->nlocal * sizeof *alocal;
    int *aremote = malloc(chunk_bytes);
    int *working = malloc(chunk_bytes);

    quicksort(alocal, args->nlocal);

    for (i = 0; i < args->nthread; i++) {
        my_barrier(args->b_phase, args->nthread);
        skip = 0;

        /* odd phase */
        if (i%2 == 1) {
            if (args->id%2 == 1 && args->id != args->nthread - 1) {
                memcpy(aremote, &args->arr[(args->id + 1) * args->nlocal], chunk_bytes);
                ks = 1;
            } else if (args->id != 0) {
                memcpy(aremote, &args->arr[(args->id - 1) * args->nlocal], chunk_bytes);
                ks = 0;
            } else { skip = 1; }
        /* even phase */
        } else if (i%2 == 0) {
            if (args->id%2 == 0 && args->id != args->nthread - 1) {
                memcpy(aremote, &args->arr[(args->id + 1) * args->nlocal], chunk_bytes);
                ks = 1;
            } else if (args->id != 0) {
                memcpy(aremote, &args->arr[(args->id - 1) * args->nlocal], chunk_bytes);
                ks = 0;
            } else { skip = 1; }
        }
        my_barrier(args->b_buf, args->nthread);
        if (!skip) {
            compare_split(args->nlocal, alocal, aremote, working, ks);
        }
    }

    free(aremote);
    free(working);
    return NULL;
}

