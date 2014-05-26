/// \file
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h> //gethostname
#include "coupling.h"
#include "layer.h"
///creates a random initial condition
///This is generated as small fluctuations away from Vrt
/// @param input    The input matrix - Modified in place
/// @param V        Used to get the Vrt 
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
        .connections = CreateCouplingMatrix(p.couple),
        .STDP_connections   = Features.STDP==ON?calloc(sizeof(Compute_float),grid_size*grid_size*couple_array_size*couple_array_size):NULL,
        .std                = STD_init(p.STD), //this is so fast that it doesn't matter to run in init
        .Extimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.single.Ex,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W>0)?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .Intimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.single.In,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W<0)?Synapse_timecourse_cache(cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .P                  = (parameters*)newdata(&p,sizeof(p)), 
        .voltages           = calloc(sizeof(Compute_float),grid_size*grid_size),
        .voltages_out       = calloc(sizeof(Compute_float),grid_size*grid_size),
    };
    for (unsigned int i=0;i<cap;i++)
    {
        L.spikes.data[i]=calloc(sizeof(coords),(grid_size*grid_size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
        L.spikes.data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
    }
    return L;
}
///The idea here is that "one-off" setup occurs here, whilst per-layer setup occurs in setuplayer
model* setup(const parameters p,const parameters p2,const LayerNumbers lcount, output_s** outputtables)
{
    const layer l1 = setuplayer(p);
    const layer l2 = lcount==DUALLAYER?setuplayer(p2):l1;
    const layer* layer1 = (layer*)newdata(&l1,sizeof(layer));//this is required to ensure that we get heap allocated layers
    const layer* layer2 = (layer*)newdata(&l2,sizeof(layer));
    const model m = {.layer1=*layer1,.layer2=*layer2,.NoLayers=lcount};
    model* m2 = malloc(sizeof(m));
    memcpy(m2,&m,sizeof(m));
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    free(buffer);
    const unsigned int output_count = 3;
    output_s* outdata=(output_s[]){ //note - neat feature - missing elements initailized to 0
        {.name="gE",.data={m2->gE,conductance_array_size,couplerange},.minval=0,.maxval=0.1}, //gE is a 'large' matrix - as it wraps around the edges
        {"gI",{m2->gI,conductance_array_size,couplerange},0,2}, //gI is a 'large' matrix - as it wraps around the edges
        {"Coupling1",{m2->layer1.connections,couple_array_size,0},0,100}, //return the coupling matrix of layer 1
        {"Coupling2",{m2->layer2.connections,couple_array_size,0},0,100}, //return the coupling matrix of layer 2
        {.minval= -INFINITY}};         //a marker that we are at the end of the outputabbles list
    output_s* malloced = malloc(sizeof(output_s)*output_count);
    memcpy(malloced,outdata,sizeof(output_s)*output_count);
    *outputtables=malloced;
    return m2;
}
