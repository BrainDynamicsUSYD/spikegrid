/// \file
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "init.h" //so I can find out the amount of ram to create
#include "utils.h"
///Creates the file for submitting the yossarian job
/// @param outfile The output filename.
/// @param sweep The parameter being swept over (only needed for the count to know how many jobs to submit)
void createyossarianfile (const char* const outfile,const sweepable sweep,const parameters L1,const parameters L2)
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
"#!/bin/bash\n"
"#PBS -N job\n"
"#PBS -P RDS-FSC-Evoked-RW\n"
"#PBS -q defaultQ\n"
//"#PBS -l select=1:ncpus=1\n"
"#PBS -l nodes=1:ppn=1\n"
"#PBS -l walltime=4:00:00\n"
"#PBS -l mem=4000MB\n"
// "#PBS -o /home/akea0933/Dump\n"
// "#PBS -e /home/akea0933/Dump\n"
//"#PBS -V\n"
"#PBS -J 0-%i\n"
"module load opencv\n"
"module load gcc\n"
"cd %s\n"
,sweep.count,cwd);
    if (sweep.count > 0)
    {
        fprintf(file,"./a.out -n -s $PBS_ARRAY_INDEX\n");
    }
    else
    {
        fprintf(file,"./a.out -n\n");
    }
    fclose(file);
}