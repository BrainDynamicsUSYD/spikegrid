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
            input[x*grid_size + y ] = ((Compute_float)random())/((Compute_float)RAND_MAX)/((Compute_float)20.0) + Param.potential.Vrt;
        }
    }
}

void setcaptests()
{   
    decay_parameters t1 = {.D=1.5,.R=0.5};
    decay_parameters t2 = {.D=2.0,.R=0.5};
    //todo:Get Adam to check these values - also add more tests
    assert (setcap(t1,(Compute_float)1E-6,Param.time.dt)==209);
    assert (setcap(t2,(Compute_float)1E-6,Param.time.dt)==270);
}

Compute_float* __attribute__((const)) Synapse_timecourse (const uint cap, const decay_parameters D)
{
    Compute_float* ret = calloc(sizeof(Compute_float),cap);
    for (unsigned int i=0;i<cap;i++)
    {
        ret[i]=(One/(D.D-D.R))*(exp(-i/D.D)-exp(-i/D.R));
    }
    return ret;
}

layer_t setuplayer(const parameters p)
{
    const unsigned int cap = max(setcap(p.synapse.Ex,1E-6,Param.time.dt),setcap(p.synapse.In,1E-6,Param.time.dt));
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
            .Extimecourse       = Synapse_timecourse(cap,p.synapse.Ex),
            .Intimecourse       = Synapse_timecourse(cap,p.synapse.In),
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
void setup()
{
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    glayer = setuplayer(Param);
}
unsigned int mytime=0;
void step(const Compute_float* const inp)
{
    mytime++;
    memcpy(glayer.voltages,inp,sizeof(float)*grid_size*grid_size);
    glayer.spikes.curidx=mytime%(glayer.spikes.count);
    step1(&glayer,mytime);
    if (Param.features.STDP==ON)
    {
        doSTDP(glayer.STDP_connections,&glayer.spikes,glayer.connections,glayer.S);
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
        {"R",{STD.R,grid_size}},
        {"U",{STD.R,grid_size}},
        {NULL}};         //a marker that we are at the end of the outputabbles list
    if (setup_done==0) 
    {
        setup();
    }
    const Compute_float* inputdata = mxGetData(prhs[0]);
    step(inputdata); //always output the voltage data
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
int main() //useful for testing w/out matlab
{
    setup();
    Compute_float* input=calloc(sizeof(Compute_float),grid_size*grid_size);
    randinit(input);
    while (mytime<100)
    {
        step(input);
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
