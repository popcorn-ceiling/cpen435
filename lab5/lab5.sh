#!/bin/bash
# file: lab5.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 3/27/15
# brief: wrapper script for lab5. fills out required table


for h in `seq 0 1`; do # sort loop
    S=$h # 0 = quicksort, 1 = bubblesort
    ELE=40000
    for i in `seq 0 1`; do # element loop
        prev=$(qsub -v ELE=$ELE,NP=1,SORT=$S job.sh)
        for j in 4 8 16; do # process loop
            cur=$(qsub -W depend=afterok:$prev -v ELE=$ELE,NP=$j,SORT=$S job.sh)
            prev=$cur
        done
        ELE=$((${ELE} * 10))
    done
done
