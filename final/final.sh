#!/bin/bash
# file: final.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 4/29/15
# brief: wrapper script for final. fills out required table

task_size=8

prev=$(qsub -v NM=512,NP=4,TS=$task_size job.sh)
for i in 8 16; do
    cur=$(qsub -W depend=afterok:$prev -v NM=512,NP=$i,TS=$task_size job.sh)
    prev=$cur
done

prev=$(qsub -W depend=afterok:$prev -v NM=1024,NP=4,TS=$task_size job.sh)
for i in 8 16; do
    cur=$(qsub -W depend=afterok:$prev -v NM=1024,NP=$i,TS=$task_size job.sh)
    prev=$cur
done
