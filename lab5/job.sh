#!/bin/bash
# filename: job.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 3/27/15
# brief: runs lab5 MPI program

#PBS -l nodes=4:ppn=4:physical
#PBS -l walltime=00:20:00
#PBS -o /home/Students/dcollins3/cpen435/lab5/${PBS_JOBID}.log
#PBS -j oe
#PBS -N lab5

echo -----------------------------------------------------------------
echo PBS: job identifier is $PBS_JOBNAME $PBS_JOBID
echo ------------------------------------------------------

date
export PROGRAM="/home/Students/dcollins3/cpen435/lab5/lab5"
mpirun -np $NP $PROGRAM $ELE $SORT
date

exit 0 
