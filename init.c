/// \file
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //gethostname
#include <sys/stat.h>
#include <stdio.h>
#include <sys/types.h>
#include "coupling.h"
#include "output.h"
#include "printstruct.h"
#include "STD.h"
///creates a random initial condition - assumes random is already seeded
///This is generated as small fluctuations away from Vrt
/// @param input    The input matrix - Modified in place
/// @param minval   The minimum value
/// @param maxval   The maximum value
void randinit(Compute_float* input,const Compute_float minval,const Compute_float maxval)
{
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = ((Compute_float)random())/((Compute_float)RAND_MAX)*(maxval-minval)+minval;
        }
    }
}
///Create an initial condition where all neurons have the same voltage except for the one in the middle
/// @param input The matrix to modify.
/// @param def_value The value for almost all the neurons
/// @param mod_value The value for the middle neuron
void Fixedinit(Compute_float* input, const Compute_float def_value,const Compute_float mod_value)
{
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = def_value;
        }
    }
    input[grid_size*(grid_size/2) + (grid_size/2)] = mod_value;
}
///Copies a struct using malloc - occasionally required
/// @param input  the initial input
/// @param size   the amount of data to copy
void* newdata(const void* const input,const unsigned int size)   
{
    void* ret = malloc(size);
    memcpy(ret,input,size);
    return ret;
} 

///given a parameters object, set up a layer object.
///This function is what theoretically will allow for sweeping through parameter space.
///The only problem is that a parameters object is immutable, so we need some way to do essentially a copy+update in C.
///Essentially, we need to be able to do something like P = {p with A=B} (F# record syntax)
///currently this function is only called from the setup function (but it could be called directly)
layer setuplayer(const parameters p)
{
    const Compute_float min_effect = (Compute_float)1E-6;
    const unsigned int STDP_cap = (unsigned int)(p.STDP.stdp_tau  * 5.0 / Features.Timestep);
    unsigned int cap;
    if (p.couple.Layertype==SINGLELAYER) {cap=max(setcap(p.couple.Layer_parameters.single.Ex,min_effect,Features.Timestep),setcap(p.couple.Layer_parameters.single.In,min_effect,Features.Timestep));}
    else                                 {cap=setcap(p.couple.Layer_parameters.dual.synapse,min_effect,Features.Timestep);}
    layer L = 
    {
        .spikes=
        {   
            .count=cap,
            .data=calloc(sizeof(coords*), cap)
        },
        .spikes_STDP = //allocate this even when STDP is off - total memory should be pretty small.  Also, default STDP_tau is 0, so this will use no memory in that case
        {
            .count=STDP_cap,
            .data=calloc(sizeof(coords*),STDP_cap)
        },
        .connections = CreateCouplingMatrix(p.couple),
        .STDP_connections   = Features.STDP==ON?calloc(sizeof(Compute_float),grid_size*grid_size*couple_array_size*couple_array_size):NULL,
        .std                = Features.STD==ON?STD_init(p.STD):NULL, //this is so fast that it doesn't matter to run in init
        .Extimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.single.Ex,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W>0)?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .Intimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.single.In,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W<0)?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .P                  = (parameters*)newdata(&p,sizeof(p)), 
        .voltages           = calloc(sizeof(Compute_float),grid_size*grid_size),
        .voltages_out       = calloc(sizeof(Compute_float),grid_size*grid_size),
        .recoverys       = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
        .recoverys_out   = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
    };
    for (unsigned int i=0;i<cap;i++)
    {
        L.spikes.data[i]=calloc(sizeof(coords),(grid_size*grid_size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
        L.spikes.data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
    }
    if (Features.STDP ==ON)
    {   //this part might actually use lots of ram, so only create if STDP is on
        for (unsigned int i=0;i<STDP_cap;i++)

        {
            L.spikes_STDP.data[i]=calloc(sizeof(coords),(grid_size*grid_size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
            L.spikes_STDP.data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
        }
    }
    return L;
}
///The idea here is that "one-off" setup occurs here, whilst per-layer setup occurs in setuplayer
// No idea wtf this function is doing - Adam.
model* setup(const parameters p,const parameters p2,const LayerNumbers lcount, int jobnumber)
{
    if (jobnumber <0)
    {
        sprintf(outdir,"output/");
        mkdir(outdir,S_IRWXU);
    }
    else
    {
        sprintf(outdir,"job-%i/",jobnumber);
        mkdir(outdir,S_IRWXU);
    }
    printf("outdir is %s\n",outdir);
    char buf[100];
    sprintf(buf,"%s/struct.dump",outdir);
    remove(buf);//cleanup the old struct file
    //printout_struct(&p,"parameters",outdir,0);     //save the first parameters object
    //printout_struct(&p2,"parameters",outdir,1);    //save the second parameters object and display everything
    const layer l1 = setuplayer(p);
    const layer l2 = lcount==DUALLAYER?setuplayer(p2):l1;
    const model m = {.layer1=l1,.layer2=l2,.NoLayers=lcount};
    model* m2 = malloc(sizeof(m));
    memcpy(m2,&m,sizeof(m));
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    free(buffer);
    output_init(m2);
 //   makeoffsets();
    return m2;
}
