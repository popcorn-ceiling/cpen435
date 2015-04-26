#!/bin/bash
# file: lab2.sh
# author: Dan Collins (dcollins3@zagmail.gonzaga.edu)
# date: 2/27/15
# brief: Automates thd/size table required for lab2

# NOTE!!! update this line for your directory
RUNDIR='/home/Students/dcollins3/cpen435/lab2'

NUM_THD=1
MAT_SIZE=40000000

while [ $NUM_THD -lt 9 ]; do
    ${RUNDIR}/lab2 $MAT_SIZE $NUM_THD
    NUM_THD=$((${NUM_THD} * 2))
done

echo

NUM_THD=1
MAT_SIZE=400000000

while [ $NUM_THD -lt 9 ]; do
    ${RUNDIR}/lab2 $MAT_SIZE $NUM_THD
    NUM_THD=$((${NUM_THD} * 2))
done

