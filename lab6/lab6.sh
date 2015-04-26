#!/bin/bash
# file: lab6.sh
# author: dcollins3 (daniel.e.collins1@zagmail.gonzaga.edu)
# date: 4/8/15
# brief: wrapper script for lab6. fills out required table

prev=$(qsub -v NM=512,NP=4 job.sh)
for i in 8 16; do
    cur=$(qsub -W depend=afterok:$prev -v NM=512,NP=$i job.sh)
    prev=$cur
done

prev=$(qsub -W depend=afterok:$prev -v NM=1024,NP=4 job.sh)
for i in 8 16; do
    cur=$(qsub -W depend=afterok:$prev -v NM=1024,NP=$i job.sh)
    prev=$cur
done
