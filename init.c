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
#include "paramheader.h"
#include "STDP.h"
#include "evolvegen.h"
#define max(a,b) \
    ({ __typeof__ (a) _a = (a);\
       __typeof__ (b) _b = (b); \
        _a>_b?_a:_b;})

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
    int cap;
    if (p.couple.Layertype==SINGLELAYER) {cap=(int)max(setcap(p.couple.Layer_parameters.single.Ex,min_effect,Features.Timestep),setcap(p.couple.Layer_parameters.single.In,min_effect,Features.Timestep));}
    else                                 {cap=(int)setcap(p.couple.Layer_parameters.dual.synapse,min_effect,Features.Timestep);}
    const int trefrac_in_ts =(int) ((Compute_float)p.couple.tref / Features.Timestep);
    const int flagcount = (int)(cap/trefrac_in_ts) + 2;
    layer L =
    {
        .firinglags         =
        {
            .lags         = calloc(sizeof(int16_t),grid_size*grid_size*(size_t)flagcount),
            .cap          = cap,
            .lagsperpoint = flagcount
        },
        .STDP_data          = Features.STDP==ON?STDP_init(p.STDP,trefrac_in_ts):NULL,
        .connections        = CreateCouplingMatrix(p.couple),
        .std                = Features.STD==ON?STD_init(p.STD):NULL,
        .Extimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.single.Ex,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W>0)?Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .Intimecourse       = p.couple.Layertype==SINGLELAYER?Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.single.In,Features.Timestep):
            ((p.couple.Layer_parameters.dual.W<0)?Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL),
        .randconns          = Features.Random_connections==ON?calloc(sizeof(randomconnection),(size_t)(grid_size*grid_size*p.random.numberper)):NULL,
        .P                  = (parameters*)newdata(&p,sizeof(p)),
        .voltages           = calloc(sizeof(Compute_float),grid_size*grid_size),
        .voltages_out       = calloc(sizeof(Compute_float),grid_size*grid_size),
        .recoverys          = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
        .recoverys_out      = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
    };
    for (int x = 0;x<grid_size;x++)
    {
        for (int    y = 0;y<grid_size;y++)
        {
            L.firinglags.lags[(x*grid_size+y)*L.firinglags.lagsperpoint]=-1;
        }
    }
    if (Features.Random_connections == ON)
    {
        //creat a rather ridiculously sized matrix
        //allows for 10x the avg number of connections per point.  Incredibly wasteful.  It would be really nice to have some c++ vectors here
        const unsigned int overkill_factor = 10;
        srandom((unsigned)0);
        for (unsigned int x=0;x<grid_size;x++)
        {
            for (unsigned int y=0;y<grid_size;y++)
            {
                for (unsigned int i=0;i<p.random.numberper;i++)
                {
                    const randomconnection rc =
                    {
                        .strength = p.random.str,
                        .stdp_strength = Zero,
                        .from = {.x=x,.y=y},
                        .destination =
                        {
                            .x = (Neuron_coord)(((Compute_float)random() / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                            .y = (Neuron_coord)(((Compute_float)random() / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                        }
                    };
                    L.randconns[(x*grid_size+y)*p.random.numberper + i] = rc;
                }
            }
        }
    }
    return L;
}
///The idea here is that "one-off" setup occurs here, whilst per-layer setup occurs in setuplayer
// No idea wtf this function is doing - Adam.
model* setup(const parameters p,const parameters p2,const LayerNumbers lcount, int jobnumber)
{
    check(); //check evolvegen   is correct
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
    printout_struct(&p,"parameters",outdir,0);     //save the first parameters object
    printout_struct(&p2,"parameters",outdir,1);    //save the second parameters object and display everything
    const layer l1  = setuplayer(p);
    const layer l2  = lcount==DUALLAYER?setuplayer(p2):l1;
    const model m   = {.layer1=l1,.layer2=l2,.NoLayers=lcount};
    model* m2       = malloc(sizeof(m));
    memcpy(m2,&m,sizeof(m));
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    free(buffer);
    output_init(m2);
    return m2;
}
