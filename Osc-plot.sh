#!/bin/bash

for file in job.o*; do
    out=$(cat $file | grep Res | awk '{print "," $2 "," $3 "," $4}' | cat -n)
    echo "$out"
done

