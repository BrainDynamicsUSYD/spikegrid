#!/bin/bash
for x in $(seq 0 100)
do
    ./a.out -n -s $x
done
