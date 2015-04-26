#!/bin/bash

#PBS -l nodes=4:ppn=2:physical
#PBS -l walltime=00:10:00
#PBS -o /home/Students/dcollins3/cpen435/lab3/mpihello.log
#PBS -j oe
#PBS -N lab3

echo -----------------------------------------------------------------
echo -n 'Job is running on node '; cat $PBS_NODEFILE
echo -----------------------------------------------------------------
echo PBS: qsub is running on $PBS_O_HOST
echo PBS: originating queue is $PBS_O_QUEUE
echo PBS: executing queue is $PBS_QUEUE
echo PBS: working directory is $PBS_O_WORKDIR
echo PBS: execution mode is $PBS_ENVIRONMENT
echo PBS: job identifier is $PBS_JOBID
echo PBS: job name is $PBS_JOBNAME
echo PBS: node file is $PBS_NODEFILE
echo PBS: current home directory is $PBS_O_HOME
echo PBS: PATH = $PBS_O_PATH
echo ------------------------------------------------------

date
export PROGRAM="/home/Students/dcollins3/cpen435/lab3/mpihello"
mpirun -np 8 $PROGRAM
date
exit 0 

