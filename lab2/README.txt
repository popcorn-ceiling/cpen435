Compile: 
    $ gcc -o lab2 sort.c seq.c -lpthread -Wextra

Memory checking:
    $ valgrind --tool=memcheck --leak-check=full ./lab2 [n] [nthreads]

Run:
    $ ./lab2 [n] [nthreads]

