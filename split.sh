#!/bin/bash
grep -i "both spiked" job.o* | tr ':-' ' ' | awk '{print $2 "," $5}' | sort -n
