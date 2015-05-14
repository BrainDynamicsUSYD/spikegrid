/// \file
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "init.h" //so I can find out the amount of ram to create
#include "utils.h"
#include "newparam.h"
///Creates the file for submitting the yossarian job
/// @param outfile The output filename.
/// @param sweep The parameter being swept over (only needed for the count to know how many jobs to submit)
void createyossarianfile (const char* const outfile,const sweepable sweep,const parameters const L1,const parameters const L2)
{
    FILE* file = fopen(outfile,"w");
    char cwd[1024];
    if (getcwd(cwd,sizeof(cwd)) == NULL)
    {
        printf("error getting current directory - exiting");
        exit(EXIT_FAILURE);
    }
    model* dummy __attribute__((unused))= setup(L1,L2,ModelType,-1,-1,1);
    int MemInMB = (int)(((double)total_malloced) * 1.1 / 1024.0 / 1024.0);
    fprintf(file,""
"#!/bin/csh\n"
"#PBS -N job\n"
"#PBS -q yossarian\n"
"#PBS -l nodes=1:ppn=1\n"
"#PBS -l walltime=200:00:00 -l mem=%iMB\n"
"#PBS -w %s\n"
"#PBS -V\n"
"#PBS -t 0-%i\n"
"cd %s\n"
"./a.out -n -s $PBS_ARRAYID\n",MemInMB,cwd,sweep.count,cwd);
    fclose(file);
}
