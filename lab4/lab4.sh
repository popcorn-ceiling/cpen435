#!/bin/bash
# file: lab4.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 3/20/15
# brief: wrapper script for lab4. fills out required table

MAT_SIZE=512
while [ $MAT_SIZE -lt 1025 ]; do
    NUM_PROCESSES=4
    while [ $NUM_PROCESSES -lt 17 ]; do
        qsub -v NM=$MAT_SIZE,NP=$NUM_PROCESSES job.sh
        NUM_PROCESSES=$((${NUM_PROCESSES} * 2))
    done
    MAT_SIZE=$((${MAT_SIZE} * 2))
done
