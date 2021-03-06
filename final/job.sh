#!/bin/bash
# filename: job.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 4/29/15
# brief: runs final MPI program

#PBS -l nodes=4:ppn=4:physical
#PBS -l walltime=00:20:00
#PBS -o /home/Students/dcollins3/cpen435/final/${PBS_JOBID}.log
#PBS -j oe
#PBS -N final

echo -----------------------------------------------------------------
echo PBS: job identifier is $PBS_JOBNAME $PBS_JOBID
echo ------------------------------------------------------

date
export PROGRAM="/home/Students/dcollins3/cpen435/final/final"
mpirun -np $NP $PROGRAM $NM $TS

exit 0 
