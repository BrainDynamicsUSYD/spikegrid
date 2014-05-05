#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "parameters.h"
void createyossarianfile (const char* const outfile)
{
    FILE* file = fopen(outfile,"w");
    char cwd[1024];
    if (getcwd(cwd,sizeof(cwd)) == NULL)
    {
        printf("error getting current directory - exiting");
        exit(EXIT_FAILURE);
    }
    fprintf(file,""
"#!/bin/csh\n"
"#PBS -N job\n"
"#PBS -q batch\n"
"#PBS -l nodes=1:ppn=1\n"
"#PBS -l walltime=2:00:00 -l mem=400MB\n"
"#PBS -w %s\n"
"#PBS -V\n"
"#PBS -t 0-%i\n"
"cd %s\n"
"./a.out -s $PBS_ARRAYID",cwd,Sweep.count,cwd);
    fclose(file);


}
