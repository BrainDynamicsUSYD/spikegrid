/// \file
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "paramheader.h"
///Creates the file for submitting the yossarian job
/// @param outfile The output filename.
/// @param sweep The parameter being swept over (only needed for the count to know how many jobs to submit)
void createyossarianfile (const char* const outfile,const sweepable sweep)
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
"./a.out -n -s $PBS_ARRAYID\n",cwd,sweep.count,cwd);
    fclose(file);


}
