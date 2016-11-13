/// \file
#include <stdlib.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h> //gethostname
#endif
#include <time.h>
#include <stdio.h>
#include "mymath.h"
#include "phi.h"
#include "coupling.h"
#include "output.h"
#include "printstruct.h"
#include "STD.h"
#include "paramheader.h"
#include "STDP.h"
#include "evolvegen.h"
#include "model.h"
#include "out/out.h"
#include "init/model.h"
#include "utils.h"
#include "animal.h"
#include "randconns.h"
#include "lagstorage.h"
#ifndef _WIN32
#define max(a,b) \
    ({ __typeof__ (a) _a = (a);\
       __typeof__ (b) _b = (b); \
        _a>_b?_a:_b;})
#endif
///creates a random initial condition - assumes random is already seeded
///This is generated as small fluctuations away from Vrt
/// @param input    The input matrix - Modified in place
/// @param minval   The minimum value
/// @param maxval   The maximum value
void randinit(Compute_float* input,const Compute_float minval,const Compute_float maxval)
{
    for (int x=0;x<grid_size*grid_size;x++)
    {
            input[x] = RandFloat()*(maxval-minval)+minval;
    }
}
///Create an initial condition where all neurons have the same voltage except for the one in the middle
/// @param input The matrix to modify.
/// @param def_value The value for almost all the neurons
/// @param mod_value The value for the middle neuron
void Fixedinit(Compute_float* input, const Compute_float def_value,const Compute_float mod_value)
{
    for (int x=0;x<grid_size*grid_size;x++)
    {
            input[x] = def_value;
    }
    input[grid_size*(grid_size/2) + (grid_size/2)] = mod_value;
}
void seedrand(const InitConds initcond,const int jobno,const int yossarianjobnumber)
{
    //seed RNG as appropriate - with either time or job number
    if     (initcond == RAND_TIME)  {srandom((unsigned)time(0));}
    else if(initcond == RAND_JOB)   {srandom((unsigned)jobno +(unsigned) (yossarianjobnumber!= -1 ?yossarianjobnumber:0 ));}
    else if(initcond == RAND_ZERO)  {srandom((unsigned)0);}
}
//sets the output directory - note output directory is a global
void PickOutputDir(const int jobnumber,const int yossarianjobnumber)
{
    if (jobnumber <0 && yossarianjobnumber <0)
    {
        sprintf(outdir,"output/");
    }
    else
    {
        char nostring[100];
        if (jobnumber < 0)
        {
            sprintf(nostring,"%i",yossarianjobnumber);
        }
        else
        {
            if (yossarianjobnumber < 0)
            {
                sprintf(nostring,"%i",jobnumber);
            }
            else
            {
                sprintf(nostring,"%i-%i",yossarianjobnumber,jobnumber);
            }
        }
        // Here it is... add the extra file.
        if (strlen(Features.Outprefix)==0)
        {
            sprintf(outdir,"job-%s/",nostring);
        }
        else
        {
            sprintf(outdir,"%s/%s/job-%s/",getenv("HOME"),Features.Outprefix,nostring);
        }
    }
    recursive_mkdir(outdir);
    printf("outdir is %s\n",outdir);
}


///The idea here is that "one-off" setup occurs here, whilst per-layer setup occurs in setuplayer
model* setup(const parameters p,const parameters p2,const LayerNumbers lcount,const int jobnumber,const int yossarianjobnumber,const int testing)
{
    PickOutputDir(jobnumber,yossarianjobnumber);
    Hook_malloc(); //this makes malloc record total number of bytes requested.
    check(); //check evolvegen   is correct
        char buf[100];
    sprintf(buf,"%s/struct.dump",outdir);
    remove(buf);//cleanup the old struct file
   // printout_struct(&p,"parameters",outdir,0);     //save the first parameters object
   // printout_struct(&p2,"parameters",outdir,1);    //save the second parameters object and display everything
    const int trefrac_in_ts =(int) ((Compute_float)p.couple.tref / Features.Timestep);
    model* m2 = makemodel(p,p2,Features,trefrac_in_ts,lcount,Extinput);
    //on the cluster, the code shouldn't be run on the main cluster node, so stop it from running
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")&& !testing) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    free(buffer);
    output_init(m2);
    MakeOutputs(Features.output);
    if (total_malloced > 1024L*1024L*1024L)
    {
        printf("Total amount of ram used: %f GB\n",((double)total_malloced) / 1024.0/1024.0/1024.0);
    }
    else
    {
        printf("Total amount of ram used: %f MB\n",((double)total_malloced) /1024.0/1024.0);
    }
    return m2;
}
