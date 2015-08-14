#!/bin/bash
data=$(grep -i "both spiked" job.o* | tr ':-' ' ' | awk '{print $6/$5 " " $5}' | sort -n)
echo "$data"
echo -e "set xlabel 'Probability of reinforcement'\n set ylabel 'trials to criterion' \n plot '-' using 1:2 smooth unique\n" "$data" "\nexit" | gnuplot -persist
