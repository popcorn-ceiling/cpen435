#!/bin/bash
# file: lab1b.sh
# author: Dan Collins (dcollins3@zagmail.gonzaga.edu)
# date: 2/20/15
# brief: Automates thd/size table required for lab1b

RUNDIR='/home/Students/dcollins3/cpen435/lab1b'

NUM_THD=1
MAT_SIZE=512

while [ $NUM_THD -lt 9 ]; do
    ${RUNDIR}/lab1b $MAT_SIZE $NUM_THD
    NUM_THD=$((${NUM_THD} * 2))
done

echo

NUM_THD=1
MAT_SIZE=1024

while [ $NUM_THD -lt 9 ]; do
    ${RUNDIR}/lab1b $MAT_SIZE $NUM_THD
    NUM_THD=$((${NUM_THD} * 2))
done

