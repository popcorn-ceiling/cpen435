#!/bin/bash
# filename: job.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 3/20/15
# brief: runs lab4 MPI program

#PBS -l nodes=4:ppn=2:physical
#PBS -l walltime=00:10:00
#PBS -o /home/Students/dcollins3/cpen435/lab4/${PBS_JOBID}.log
#PBS -j oe
#PBS -N lab4

echo -----------------------------------------------------------------
echo PBS: job name is $PBS_JOBNAME
echo PBS: job identifier is $PBS_JOBID
echo PBS: qsub is running on $PBS_O_HOST
echo PBS: working directory is $PBS_O_WORKDIR
echo ------------------------------------------------------

date
export PROGRAM="/home/Students/dcollins3/cpen435/lab4/lab4"
mpirun -np $NP $PROGRAM $NM
date

exit 0 
