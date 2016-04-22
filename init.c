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
///given a parameters object, set up a layer object.
///currently this function is only called from the setup function (but it could be called directly)
layer setuplayer(const parameters p)
{
    const Compute_float min_effect = (Compute_float)1E-6;
    int cap; //this cap should really be calculated in lagstorage init - it doesn't feel quite appropriate here
    if (p.couple.Layertype==SINGLELAYER) {cap=(int)max(setcap(p.couple.Layer_parameters.single.Ex,min_effect,Features.Timestep),setcap(p.couple.Layer_parameters.single.In,min_effect,Features.Timestep));}
    else                                 {cap=(int)setcap(p.couple.Layer_parameters.dual.synapse,min_effect,Features.Timestep);}
    const int trefrac_in_ts =(int) ((Compute_float)p.couple.tref / Features.Timestep);
    unsigned int flagcount;
    if (Features.Recovery == ON) {flagcount = (unsigned)cap;} //this needs a comment
    else {flagcount = (unsigned)(cap/trefrac_in_ts) + 1 + 1;} //1 for possible fencepost problem. 1 for the space for the -1 at the end
    parameters* P = (parameters*)newdata(&p,sizeof(p));
    layer L =
    {   //I am not particularly happy with this block.  It is highly complicated.  One idea: have the init functions themselves decide to return null
        .firinglags         = lagstorage_init(flagcount,cap), //TODO: this can be reduced as we now only need to keep track of things for the refractory period
        .STDP_data          = Features.STDP==ON?STDP_init(&P->STDP,trefrac_in_ts):NULL, //problem - P defd later
        .connections        = CreateCouplingMatrix(p.couple),
        .std                = Features.STD==ON?STD_init(p.STD):NULL,
        .Phimat             = (p.Stim.Periodic==OFF && Features.ImageStim==ON)?CreatePhiMatrix():NULL,
        .Extimecourse       = p.couple.Layertype==SINGLELAYER?
            Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.single.Ex,Features.Timestep):NULL,
        .Intimecourse       = p.couple.Layertype==SINGLELAYER?
            Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.single.In,Features.Timestep):NULL,
        .Mytimecourse       = p.couple.Layertype==DUALLAYER?
           Synapse_timecourse_cache((unsigned int)cap,p.couple.Layer_parameters.dual.synapse,Features.Timestep):NULL,
        .P                  = P,
        .voltages           = calloc(sizeof(Compute_float),grid_size*grid_size),
        .voltages_out       = calloc(sizeof(Compute_float),grid_size*grid_size),
        .recoverys          = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
        .recoverys_out      = Features.Recovery==ON?calloc(sizeof(Compute_float),grid_size*grid_size):NULL,
        .Layer_is_inhibitory = p.couple.Layertype==DUALLAYER && p.couple.Layer_parameters.dual.W<0,
        .rcinfo             = Features.Random_connections==ON?init_randconns(p.random,p.couple): NULL,
        .RD                 = calloc(sizeof(RD_data),1),
    };
    L.RD->R=p.couple.Layer_parameters.dual.synapse.R;
    L.RD->D=p.couple.Layer_parameters.dual.synapse.D;
    return L;
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
    const layer l1  = setuplayer(p);
    const layer l2  = lcount==DUALLAYER?setuplayer(p2):l1;
    condmat* condmatinit = calloc(sizeof(condmat),1);
    for (Neuron_coord i=0;i<grid_size;i++)
    {
        for (Neuron_coord j=0;j<grid_size;j++)
        {
            const size_t idx = Conductance_index((coords){.x=i,.y=j});
            condmatinit->gI[idx]=Extinput.gI0;
            condmatinit->gE[idx]=Extinput.gE0;
        }
    }

#ifdef _WIN32 //this might be a bug in VS - maybe only intellisense?
    model m   = {l1,l2,0,lcount,calloc(sizeof(animal),1)};
#else
    //we construct this model as some of the features are const
    //as a result, as some of the objects are const, they can't be copied in to the result -
    //model* m = malloc(sizeof(*m));
    //m->layer1 = ...   will fail
    //this pattern is used in a few places
	const model m =
        {
            .layer1 = l1,
            .layer2 = l2,
            .NoLayers = lcount,
            .animal = calloc(sizeof(animal),1),
            .cond_matrices = calloc(sizeof(condmat),1),
            .cond_matrices_init = condmatinit,
            .timesteps = 0 };
#endif
    model* m2       = malloc(sizeof(m));
    memcpy(m2,&m,sizeof(m));
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
