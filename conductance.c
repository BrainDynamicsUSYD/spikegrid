#include <stdio.h> //printf
#include <stdlib.h> //malloc/calloc etc  random/srandom
#include <time.h>   //time - for seeding RNG
#include <string.h> //memcpy
#include <unistd.h> //gethostname
#include <getopt.h> //getopt
#include "matlab_includes.h"
#include "paramheader.h"
#include "helpertypes.h"
#include "coupling.h"
#include "STDP.h"
#include "STD.h"
#include "movie.h"
#include "output.h"
#include "assert.h"
#include "evolve.h"
#include "newparam.h"
#include "yossarian.h"
//creates a random initial condition - small fluctuations away from Vrt
void randinit(Compute_float* input)
{
    srandom((unsigned)(time(0)));
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = ((Compute_float)random())/((Compute_float)RAND_MAX)/((Compute_float)20.0) + Param.potential.Vrt;
        }
    }
}

//some tests that the setcap function is correct.
//TODO: add more tests
void setcaptests()
{   
    decay_parameters t1 = {.D=1.5,.R=0.5};
    decay_parameters t2 = {.D=2.0,.R=0.5};
    //todo:Get Adam to check these values - also add more tests
    assert (setcap(t1,(Compute_float)1E-6,Param.time.dt)==209);
    assert (setcap(t2,(Compute_float)1E-6,Param.time.dt)==270);
}
//The spikes that we emit have a time course.  This function calculates the timecourse and returns an array of cached values to avoid recalculating at every timestep
Compute_float* __attribute__((const)) Synapse_timecourse_cache (const uint cap, const decay_parameters Decay,const time_parameters t)
{
    Compute_float* ret = calloc(sizeof(Compute_float),cap);
    for (unsigned int i=0;i<cap;i++)
    {
        const Compute_float time = ((Compute_float)i)*t.dt;
        ret[i]=Synapse_timecourse(Decay,time); 
    }
    return ret;
}
//given a parameters object, set up a layer object.
//This function is what theoretically will allow for sweeping through parameter space.
//The only problem is that a parameters object is immutable, so we need some way to do essentially a copy+update in C.
//Essentially, we need to be able to do something like P = {p with A=B} (F# record syntax)
//currently this function is only called from the setup function (but it could be called directly)
layer_t setuplayer(const parameters p)
{
    const Compute_float min_effect = (Compute_float)1E-6;
    const unsigned int cap = max(setcap(p.synapse.Ex,min_effect,Param.time.dt),setcap(p.synapse.In,min_effect,Param.time.dt));
    layer_t layer = 
        {
            .spikes=
            {   
                .count=cap,
                .data=calloc(sizeof(coords*), cap)
            },
            .connections = CreateCouplingMatrix(p.couple),
            .STDP_connections = p.features.STDP==ON?calloc(sizeof(Compute_float),grid_size*grid_size*couple_array_size*couple_array_size):NULL,
            .std                = STD_init(&p.STD), //this is so fast that it doesn't matter to run in init
            .Extimecourse       = Synapse_timecourse_cache(cap,p.synapse.Ex,p.time),
            .Intimecourse       = Synapse_timecourse_cache(cap,p.synapse.In,p.time),
            .P                  = &(p.potential),
            .S                  = &(p.STDP)
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

layer_t glayer;
//The idea here is that "one-off" setup occurs here, whilst per-layer setup occurs in setuplayer
void setup(const parameters p)
{
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    glayer = setuplayer(p);
}
unsigned int mytime=0;
//The step function - evolves the model through time.
//Perf wise the memcpy is probably not ideal, but this is a simple setup and the perf loss here is pretty small as memcpy is crazy fast
//DO NOT CALL THIS FUNCTION "step" - this causes a weird collision in matlab that results in segfaults.  Incredibly fun to debug
void step_(const Compute_float* const inp)
{
    mytime++;
    memcpy(glayer.voltages,inp,sizeof(float)*grid_size*grid_size);
    glayer.spikes.curidx=mytime%(glayer.spikes.count);
    step1(&glayer,mytime);
    if (Param.features.STDP==ON)
    {
        doSTDP(glayer.STDP_connections,&glayer.spikes,glayer.connections,glayer.S,&Param.features);
    }
    if (Param.features.Movie==ON &&  mytime % Param.Movie.Delay == 0) {printVoltage(glayer.voltages_out);}
   
}

#ifdef MATLAB
int setup_done=0;
//function called by matlab
//currently does no checking on input / output, so if you screw up your matlab expect segfaults
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
    output_s Outputabble[]={ //note - neat feature - missing elements initailized to 0
        {"gE",{GE,conductance_array_size,couplerange}}, //gE is a 'large' matrix - as it wraps around the edges
        {"gI",{GI,conductance_array_size,couplerange}}, //gI is a 'large' matrix - as it wraps around the edges
        {"R",{glayer.std.R,grid_size}},
        {"U",{glayer.std.R,grid_size}},
        {NULL}};         //a marker that we are at the end of the outputabbles list
    if (setup_done==0) 
    {
        setup(Param);
        setup_done=1;
        printf("setup done\n");
    }
    const Compute_float* inputdata = (Compute_float*) mxGetData(prhs[0]);
    step_(inputdata); //always output the voltage data
    plhs[0]=mxCreateNumericMatrix(grid_size,grid_size,mxSINGLE_CLASS,mxREAL);
    Compute_float* pointer=(Compute_float*)mxGetPr(plhs[0]);
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            pointer[i*grid_size+j]=glayer.voltages_out[i*grid_size + j];
        }
    }
    if (nrhs>1) //output other stuff
    {
        int rhsidx = 1;
        while (rhsidx<nrhs)
        {
            char* data=malloc(sizeof(char)*1024); //should be big enough
            mxGetString(prhs[rhsidx],data,1023);
            int outidx = 0;
            while (Outputabble[outidx].name != NULL)
            {
                if (!strcmp(Outputabble[outidx].name,data))
                {
                    plhs[rhsidx]=outputToMxArray(Outputabble[outidx].data);
                    outidx=-1;
                    break;
                }
                outidx++;
            }
            if (outidx != -1) {printf("UNKNOWN THING TO OUTPUT\n");}
            free(data);
            rhsidx++;
        }
    }
}
#endif
void tests()
{
    setcaptests();
    testmodparam(Param);
    printf("tests passed");
}
struct option long_options[] = {{"help",no_argument,0,'h'},{"generate",no_argument,0,'g'},{"sweep",required_argument,0,'s'}};
int main(int argc,char** argv) //useful for testing w/out matlab
{
    int c;
    int skiptests=0;
    while (1)
    {
        int option_index=0;
        c=getopt_long(argc,argv,"hgs:",long_options,&option_index);
        if (c==-1) {break;} //end of options
        switch (c)
        {
            case 'h':
                printf("available arguments:\n"
                        "   -h --help print this message\n"
                        "   -g --generate generate a yossarian config file\n"
                        "   -s --sweep N do the nth element of a sweep\n");
                exit(EXIT_SUCCESS);
            case 'g':
                createyossarianfile("yossarian.csh");
                exit(EXIT_SUCCESS);
            case 's':
                {
                    const int index=atoi(optarg);
                    printf("doing sweep index %i\n",index);
                    const parameters newparam = GetNthParam(Param,Sweep,index);
                    skiptests=1;
                    setup(newparam);
                }
                break;
        }
    }
    if (skiptests==0){tests();}
    setup(Param);
    Compute_float* input=calloc(sizeof(Compute_float),grid_size*grid_size);
    randinit(input);
    while (mytime<1000)
    {
        step_(input);
        printf("%i\n",mytime);
        for (int i=0;i<grid_size;i++)
        {
            for (int j=0;j<grid_size;j++)
            {
                input[i*grid_size+j]=glayer.voltages_out[i*grid_size + j];
            }
        }
    }
    free(input);
    return(EXIT_SUCCESS);
}
