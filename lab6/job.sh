#!/bin/bash
# filename: job.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 4/8/15
# brief: runs lab6 MPI program

#PBS -l nodes=4:ppn=4:physical
#PBS -l walltime=00:10:00
#PBS -o /home/Students/dcollins3/cpen435/lab6/${PBS_JOBID}.log
#PBS -j oe
#PBS -N lab6

echo -----------------------------------------------------------------
echo PBS: job identifier is $PBS_JOBNAME $PBS_JOBID
echo ------------------------------------------------------

date
export PROGRAM="/home/Students/dcollins3/cpen435/lab6/lab6"
mpirun -np $NP $PROGRAM $NM

exit 0 
