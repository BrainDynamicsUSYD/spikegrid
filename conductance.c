#include "matlab_includes.h"
#include "ringbuffer.h"
#include "parameters.h"
#include "helpertypes.h"
#include "coupling.h"
#include "STDP.h"
#include "STD.h"
#include "movie.h"
#include "output.h"
#include "assert.h"
#include "evolve.h"
#include "layer.h"
#include <tgmath.h> //exp
#include <stdio.h> //printf
#include <stdlib.h> //malloc/calloc etc  random/srandom
#include <time.h>   //time - for seeding RNG
#include <string.h> //memcpy
#include <unistd.h> //gethostname


//creates a random initial condition - small fluctuations away from Vrt
void randinit(Compute_float* input)
{
    srandom((unsigned)(time(0)));
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = ((Compute_float)random())/((Compute_float)RAND_MAX)/20.0 + Param.potential.Vrt;
        }
    }
}


void setcaptests()
{   
    //todo:Get Adam to check these values
    assert (setcap(1.5,0.5,1E-6)==209);
    assert (setcap(2.0,0.5,1E-6)==270);
}

layer_t glayer;
//allocate memory - that sort of thing
void setup()
{
    couple_array_size=2*couplerange+1;
    //compute some constants
    int cap=max(setcap(Param.synapse.taudE,Param.synapse.taurE,1E-6),setcap(Param.synapse.taudI,Param.synapse.taurI,1E-6));
    //set up our data structure to store spikes
    glayer.spikes.count=cap;
    glayer.spikes.data=calloc(sizeof(coords*), cap);
    glayer.connections        = CreateCouplingMatrix();
    glayer.STDP_connections   = calloc(sizeof(Compute_float),grid_size*grid_size*couple_array_size*couple_array_size);
    memset(glayer.voltages,0,grid_size*grid_size);
    memset(glayer.voltages_out,0,grid_size*grid_size);
    for (int i=0;i<cap;i++)
    {
        glayer.spikes.data[i]=calloc(sizeof(coords),(grid_size*grid_size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
        glayer.spikes.data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
    }
    //for storing voltages
    if (Param.features.STD == ON) {STD_init();}

}
int mytime=0;
void matlab_step(const Compute_float* const inp)
{
    mytime++;
    memcpy(glayer.voltages,inp,sizeof(float)*grid_size*grid_size);
    glayer.spikes.curidx=mytime%(glayer.spikes.count);
    step1(&glayer,mytime);
    if (Param.features.STDP==ON)
    {
        doSTDP(glayer.STDP_connections,glayer.spikes,glayer.connections);
    }
    if (Param.features.Movie==ON &&  mytime % Param.Movie.Delay == 0) {printVoltage(glayer.voltages_out);}
   
}
int setup_done=0;
#ifdef MATLAB
//some classes for returning the data to matlab

output_s Outputabble[]={ //note - neat feature - missing elements initailized to 0
    {"gE",{glayer.gE,conductance_array_size,couplerange}}, //gE is a 'large' matrix - as it wraps around the edges
    {"gI",{glayer.gI,conductance_array_size,couplerange}}, //gE is a 'large' matrix - as it wraps around the edges
    {"R",{STD.R,grid_size}},
    {"U",{STD.R,grid_size}},
    {NULL}};         //a marker that we are at the end of the outputabbles list
//function called by matlab
//currently does no checking on input / output, so if you screw up your matlab expect segfaults
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
    if (setup_done==0) 
    {
        char* buffer = malloc(1024);
        gethostname(buffer,1023);
        if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
        printf("setup started\n");setup();setup_done=1;printf("done setup\n");
    }
    const Compute_float* inputdata = mxGetData(prhs[0]);
    matlab_step(inputdata);
    plhs[0]=mxCreateNumericMatrix(grid_size,grid_size,mxSINGLE_CLASS,mxREAL);
    Compute_float* pointer=(Compute_float*)mxGetPr(plhs[0]);
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            pointer[i*grid_size+j]=glayer.voltages_out[i*grid_size + j];
        }
        printf("%f\n",glayer.voltages_out[i*grid_size]);
    }
    if (nrhs>1)
    {
        int rhsidx = 1;
        while (rhsidx<nrhs)
        {
            char* data=malloc(sizeof(char)*1024);
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
int main()
{
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    setup();
    Compute_float* input=calloc(sizeof(Compute_float),grid_size*grid_size);
    randinit(input);
    while (mytime<100)
    {
        matlab_step(input);
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
    return EXIT_SUCCESS;
}
