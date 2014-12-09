/// \file
#include <stdlib.h>
#include <string.h>
#include <unistd.h> //gethostname
#include <stdio.h>
#include "coupling.h"
#include "output.h"
#include "printstruct.h"
#include "STD.h"
#include "paramheader.h"
#include "STDP.h"
#include "evolvegen.h"
#include "model.h"
#include "out/out.h"
#include "utils.h"
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
    for (int x=0;x<grid_size*grid_size;x++)
    {
            input[x] = ((Compute_float)(random()))/((Compute_float)RAND_MAX)*(maxval-minval)+minval;
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
        .firinglags         = lagstorage_init(flagcount,cap),
        .STDP_data          = Features.STDP==ON?STDP_init(p.STDP,trefrac_in_ts):NULL,
        .connections        = CreateCouplingMatrix(p.couple),
        .std                = Features.STD==ON?STD_init(p.STD):NULL,
        .Extimecourse       = p.couple.Layertype==SINGLELAYER?
            Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.single.Ex,Features.Timestep):NULL,
        .Intimecourse       = p.couple.Layertype==SINGLELAYER?
            Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.single.In,Features.Timestep):NULL,
        .Mytimecourse       = p.couple.Layertype==DUALLAYER?
           Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL,
        .rcinfo =
            {
                .randconns          = Features.Random_connections==ON?
                    calloc(sizeof(randomconnection),(size_t)(grid_size*grid_size*p.random.numberper)):NULL
            },
        .P                  = (parameters*)newdata(&p,sizeof(p)),
        .voltages           = calloc(sizeof(Compute_float),grid_size*grid_size),
        .voltages_out       = calloc(sizeof(Compute_float),grid_size*grid_size),
        .recoverys          = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
        .recoverys_out      = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
        .Layer_is_inhibitory = p.couple.Layertype==DUALLAYER && p.couple.Layer_parameters.dual.W<0,
    };
    //the next section deals with setup for random connsections.  It is quite messy and long.
    //much of this complexity is involved with allowing access to the random connections in efficient ways
    //(particularly when STDP is included in the model)
    if (Features.Random_connections == ON)
    {
        //creat a rather ridiculously sized matrix
        //allows for 10x the avg number of connections per point.  Incredibly wasteful.  It would be really nice to have some c++ vectors here
        const unsigned int overkill_factor = 10;
        randomconnection** bigmat = calloc(sizeof(randomconnection*),grid_size*grid_size*p.random.numberper*overkill_factor);
        unsigned int* bigmatcounts = calloc(sizeof(unsigned int),grid_size*grid_size);
        int nonzcount;
        Compute_float* interestingconns;
        Non_zerocouplings(p.couple,&interestingconns,&nonzcount);
        srandom((unsigned)0);
        for (unsigned int x=0;x<grid_size;x++)
        {
            for (unsigned int y=0;y<grid_size;y++)
            {
                for (unsigned int i=0;i<p.random.numberper;i++)
                {
                    const randomconnection rc =
                    {
                        .strength = interestingconns[random()%nonzcount] * (One - p.couple.normalization_parameters.glob_mult.GM),
                        .stdp_strength = Zero,
                        .destination =
                        {
                            .x = (Neuron_coord)(((Compute_float)(random()) / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                            .y = (Neuron_coord)(((Compute_float)(random()) / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                        }
                    };
                    L.rcinfo.randconns[(x*grid_size+y)*p.random.numberper + i] = rc;
                    //the normal matrix stores by where they come from.  Also need to store where they got to.
                    bigmat[(rc.destination.x*grid_size+rc.destination.y)*(int)p.random.numberper*(int)overkill_factor + (int)bigmatcounts[rc.destination.x*grid_size+rc.destination.y]]=&L.rcinfo.randconns[(x*grid_size+y)*p.random.numberper + i];
                    bigmatcounts[rc.destination.x*grid_size+rc.destination.y]++;
                    if(bigmatcounts[rc.destination.x*grid_size+rc.destination.y] > overkill_factor*p.random.numberper)
                    {
                        printf("Overkill factor is not large enough - please make it bigger at dx = %i dy = %i\n",rc.destination.x,rc.destination.y);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
        randomconnection** rev_conns = malloc(sizeof(randomconnection*)*grid_size*grid_size*p.random.numberper);
        randomconnection*** rev_conns_lookup = malloc(sizeof(randomconnection**)*grid_size*grid_size);
        unsigned int* rev_pp = malloc(sizeof(unsigned int)*grid_size*grid_size);
        int count = 0;

        for (unsigned int x=0;x<grid_size;x++)
        {
            for (unsigned int y=0;y<grid_size;y++)
            {
                rev_conns_lookup[x*grid_size+y] = &rev_conns[count];
                unsigned int mycount = 0;
                while(bigmat[(x*grid_size+y)*p.random.numberper*overkill_factor + mycount] != NULL && mycount < overkill_factor)
                {
                    rev_conns[count]=bigmat[(x*grid_size+y)*p.random.numberper*overkill_factor + mycount];
                    count++;mycount++;
                }
                rev_pp[x*grid_size+y]=mycount;
            }
        }
        L.rcinfo.rev_pp=rev_pp;
        L.rcinfo.randconns_reverse=rev_conns;
        L.rcinfo.randconns_reverse_lookup = rev_conns_lookup;
        free(bigmat);
        free(bigmatcounts);
    }
    return L;
}

///The idea here is that "one-off" setup occurs here, whilst per-layer setup occurs in setuplayer
model* setup(const parameters p,const parameters p2,const LayerNumbers lcount, int jobnumber)
{
    check(); //check evolvegen   is correct
    if (jobnumber <0)
    {
        sprintf(outdir,"output/");
    }
    else
    {
        // Here it is... add the extra file.
        if (strlen(Features.Outprefix)==0)
        {
            sprintf(outdir,"job-%i/",jobnumber);
        }
        else
        {
            sprintf(outdir,"%s/%s/job-%i/",getenv("HOME"),Features.Outprefix,jobnumber);
        }
    }
    recursive_mkdir(outdir);

    printf("outdir is %s\n",outdir);
    char buf[100];
    sprintf(buf,"%s/struct.dump",outdir);
    remove(buf);//cleanup the old struct file
   // printout_struct(&p,"parameters",outdir,0);     //save the first parameters object
   // printout_struct(&p2,"parameters",outdir,1);    //save the second parameters object and display everything
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
    MakeOutputs(p.output);
    if (lcount==DUALLAYER) {MakeOutputs(p2.output);}
    return m2;
}
