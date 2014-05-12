#include <stdio.h> //printf
#include <string.h> //memcpy
#include <getopt.h> //getopt
#include "STDP.h"
#include "evolve.h"
#include "newparam.h"
#include "init.h"
#include "yossarian.h"
#include "coupling.h"
unsigned int mytime=0;
//The step function - evolves the model through time.
//Perf wise the memcpy is probably not ideal, but this is a simple setup and the perf loss here is pretty small as memcpy is crazy fast
//DO NOT CALL THIS FUNCTION "step" - this causes a weird collision in matlab that results in segfaults.  Incredibly fun to debug
///Function that steps the model through time (high level).
///TODO: Currently broken for single layer model
/// @param inp the input voltages
void step_(const Compute_float* const inp)
{
    mytime++;
    memcpy(glayer.voltages,inp,sizeof(float)*grid_size*grid_size);
    glayer.spikes.curidx=mytime%(glayer.spikes.count);
    step1(&glayer,mytime);
    step1(&glayer2,mytime);
    if (Features.STDP==ON)
    {
        doSTDP(glayer.STDP_connections,&glayer.spikes,glayer.connections,glayer.P->STDP);
        doSTDP(glayer2.STDP_connections,&glayer2.spikes,glayer2.connections,glayer2.P->STDP);
    }
    makemovie(glayer,mytime);
    makemovie(glayer2,mytime);
}


#ifdef MATLAB
int setup_done=0;
//function called by matlab
//currently does no checking on input / output, so if you screw up your matlab expect segfaults
//
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
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
#else
void tests()
{
    setcaptests();
    testmodparam(OneLayerModel);
    printf("tests passed");
}
struct option long_options[] = {{"help",no_argument,0,'h'},{"generate",no_argument,0,'g'},{"sweep",required_argument,0,'s'}};
int main(int argc,char** argv) //useful for testing w/out matlab
{
    int skiptests=0;
    while (1)
    {
        int option_index=0;
        int c=getopt_long(argc,argv,"hgs:",long_options,&option_index);
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
                createyossarianfile("yossarian.csh",Sweep);
                exit(EXIT_SUCCESS);
            case 's':
                {
                    const int index=atoi(optarg);
                    printf("doing sweep index %i\n",index);
                    if (ModelType == SINGLELAYER)
                    {
                        const parameters newparam = GetNthParam(OneLayerModel,Sweep,index);
                        skiptests=1;
                        setup(newparam);
                    }
                    else {printf("sweeps not currently supported in dual-layer model\n");exit(EXIT_FAILURE);}
                }
                break;
        }
    }
    if (skiptests==0){tests();}
    if (ModelType==SINGLELAYER) {setup(OneLayerModel);} 
    else {setup(DualLayerModelIn);setup(DualLayerModelEx);}
    Compute_float* input=calloc(sizeof(Compute_float),grid_size*grid_size);
    randinit(input,OneLayerModel.potential); //need to fix for dual layer
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
#endif
