#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <assert.h>
#include "coupling.h"
#include "STD.h"
#include "init.h"
#include "evolve.h"
//creates a random initial condition - small fluctuations away from Vrt
void randinit(Compute_float* input,const conductance_parameters V)
{
    srandom((unsigned)(time(0)));
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = ((Compute_float)random())/((Compute_float)RAND_MAX)/((Compute_float)20.0) + V.Vrt;
        }
    }
}
//The spikes that we emit have a time course.  This function calculates the timecourse and returns an array of cached values to avoid recalculating at every timestep
Compute_float* __attribute__((const)) Synapse_timecourse_cache (const unsigned int cap, const decay_parameters Decay,const Compute_float timestep)
{
    Compute_float* ret = calloc(sizeof(Compute_float),cap);
    for (unsigned int i=0;i<cap;i++)
    {
        const Compute_float time = ((Compute_float)i)*timestep;
        ret[i]=Synapse_timecourse(Decay,time); 
    }
    return ret;
}

//some tests that the setcap function is correct.
//TODO: add more tests
void setcaptests()
{   
    //todo:Get Adam to check these values - also add more tests
    assert (setcap((decay_parameters){.D=1.5,.R=0.5},(Compute_float)1E-6,Features.Timestep)==209);
    assert (setcap((decay_parameters){.D=2.0,.R=0.5},(Compute_float)1E-6,Features.Timestep)==270);
}

//given a parameters object, set up a layer object.
//This function is what theoretically will allow for sweeping through parameter space.
//The only problem is that a parameters object is immutable, so we need some way to do essentially a copy+update in C.
//Essentially, we need to be able to do something like P = {p with A=B} (F# record syntax)
//currently this function is only called from the setup function (but it could be called directly)
layer_t setuplayer(const parameters p)
{
    const Compute_float min_effect = (Compute_float)1E-6;
    unsigned int cap;
    if (p.couple.Layertype==SINGLELAYER) {cap=max(setcap(p.couple.Layer_parameters.single.Ex,min_effect,Features.Timestep),setcap(p.couple.Layer_parameters.single.In,min_effect,Features.Timestep));}
    else                                 {cap=setcap(p.couple.Layer_parameters.dual.synapse,min_effect,Features.Timestep);}
    layer_t layer = 
    {
        .spikes=
        {   
            .count=cap,
            .data=calloc(sizeof(coords*), cap)
        },
        .connections = CreateCouplingMatrix(p.couple),
        .STDP_connections   = Features.STDP==ON?calloc(sizeof(Compute_float),grid_size*grid_size*couple_array_size*couple_array_size):NULL,
        .std                = STD_init(p.STD), //this is so fast that it doesn't matter to run in init
        .Extimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.single.Ex,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W>0)?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .Intimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.single.In,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W<0)?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .P                  = (parameters*)newdata(&p,sizeof(p)), 
    };

    memset(layer.voltages,0,grid_size*grid_size); //probably not required
    memset(layer.voltages_out,0,grid_size*grid_size);//probably not required
    for (unsigned int i=0;i<cap;i++)
    {
        layer.spikes.data[i]=calloc(sizeof(coords),(grid_size*grid_size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
        layer.spikes.data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
    }
    return layer;
}
int setuplayerone=0;
//The idea here is that "one-off" setup occurs here, whilst per-layer setup occurs in setuplayer
void setup(const parameters p)
{
    if (setuplayerone==0) {glayer = setuplayer(p);setuplayerone++;}
    else                  {glayer2= setuplayer(p);}
}
void init()
{
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    //TODO: minval/maxvals are made up - need to fix
    Outputtable=(output_s[]){ //note - neat feature - missing elements initailized to 0
        {"gE",{GE,conductance_array_size,couplerange},0,100}, //gE is a 'large' matrix - as it wraps around the edges
        {"gI",{GI,conductance_array_size,couplerange},0,100}, //gI is a 'large' matrix - as it wraps around the edges
        {"R",{glayer.std.R,grid_size,0},0,100},
        {"U",{glayer.std.R,grid_size,0},0,100},
        {NULL}};         //a marker that we are at the end of the outputabbles list
}