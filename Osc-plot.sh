#!/bin/bash

for file in job.o*; do
    out=$(cat $file | grep Res | awk '{print ","  $4 "," $5}' | cat -n)
    echo "$out"
done

