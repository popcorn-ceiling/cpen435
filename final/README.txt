Compile: 
    $ mpicc -o final final.c ./threadpool/src/threadpool.c -Wextra -lpthread

Run:
    $ final.sh

Log file:
    <job_id>.log
    cat *.log > out.log
