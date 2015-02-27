#!/bin/bash

out=vg.out
function vg(){
    valgrind --leak-check=full -v --show-reachable=yes --track-origins=yes "$@"
}

function jp(){
    vg ./json-parser -f "$@"
}

echo "Running tests... this will take a while... output is in [$out]..."
{
    echo 'Running basic sanity tests...'
    vg ./test
    echo "Running must-fail tests..."
    for i in checker/test/fail*.json; do jp $i; done
    echo "Running must-pass tests..."
    for i in checker/test/pass*.json; do jp $i; done
} &> $out

echo "Done... grepping..."

grep 'ERROR SUMMARY:' $out
grep 'total heap usage' $out

echo "Output is in [$out]."
