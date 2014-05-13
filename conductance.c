#include <stdio.h> //printf
#include <string.h> //memcpy
#include <getopt.h> //getopt
#include "mymath.h"
#include "STDP.h"
#include "evolve.h"
#include "newparam.h"
#include "init.h"
#include "yossarian.h"
#include "coupling.h"
unsigned int mytime=0;
model* m;
//The step function - evolves the model through time.
//Perf wise the memcpy is probably not ideal, but this is a simple setup and the perf loss here is pretty small as memcpy is crazy fast
//DO NOT CALL THIS FUNCTION "step" - this causes a weird collision in matlab that results in segfaults.  Incredibly fun to debug
///Function that steps the model through time (high level).
/// @param inp the input voltages
/// @param inp2 input voltages for layer 2.  In the single layer model a dummy argument needs to be passed.
void step_(const Compute_float* const inp,const Compute_float* const inp2)
{
    if (ModelType==DUALLAYER && inp2==NULL) {printf("missing second input voltage in dual-layer model");exit(EXIT_FAILURE);}
    mytime++;
    memcpy(m->layer1.voltages,inp,sizeof(float)*grid_size*grid_size);
    if (ModelType==DUALLAYER) {memcpy(m->layer2.voltages,inp,sizeof(float)*grid_size*grid_size);}
    m->layer1.spikes.curidx=mytime%(m->layer1.spikes.count);
    if (ModelType==DUALLAYER) {m->layer2.spikes.curidx=mytime%(m->layer2.spikes.count);}
    step1(m,mytime);
    if (Features.STDP==ON)
    {
        doSTDP(m->layer1.STDP_connections,&m->layer1.spikes,m->layer1.connections,m->layer1.P->STDP);
        doSTDP(m->layer2.STDP_connections,&m->layer2.spikes,m->layer2.connections,m->layer2.P->STDP);
    }
    makemovie(m->layer1,mytime);
    makemovie(m->layer2,mytime);
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
        if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType);} //pass the same layer as a double parameter
        else {m=setup(DualLayerModelIn,DualLayerModelEx,ModelType);}
        setup_done=1;
        printf("setup done\n");
    }
    const Compute_float* inputdata = (Compute_float*) mxGetData(prhs[0]);
    const Compute_float* inputdata2 = (Compute_float*) mxGetData(prhs[1]);
    step_(inputdata,inputdata2); //always output the voltage data
    plhs[0]=mxCreateNumericMatrix(grid_size,grid_size,mxSINGLE_CLASS,mxREAL);
    plhs[1]=mxCreateNumericMatrix(grid_size,grid_size,mxSINGLE_CLASS,mxREAL);
    Compute_float* pointer=(Compute_float*)mxGetPr(plhs[0]);
    Compute_float* pointer2=(Compute_float*)mxGetPr(plhs[1]);
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            pointer[i*grid_size+j]=m->layer1.voltages_out[i*grid_size + j];
            pointer2[i*grid_size+j]=m->layer2.voltages_out[i*grid_size + j];
        }
    }
    if (nrhs>1) //output other stuff
    {
        int rhsidx = 2;
        while (rhsidx<nrhs)
        {
            char* data=malloc(sizeof(char)*1024); //should be big enough
            mxGetString(prhs[rhsidx],data,1023);
            int outidx = 0;
            while (Outputtable[outidx].minval != -INFINITY)
            {
                if (!strcmp(Outputtable[outidx].name,data))
                {
                    plhs[rhsidx]=outputToMxArray(Outputtable[outidx].data);
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
                        //TODO: sweep currently broken
                     //   const parameters newparam = GetNthParam(OneLayerModel,Sweep,index);
                        skiptests=1;
                    }
                    else {printf("sweeps not currently supported in dual-layer model\n");exit(EXIT_FAILURE);}
                }
                break;
        }
    }
    if (skiptests==0){tests();}
    if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType);} //pass the same layer as a double parameter
    else {m=setup(DualLayerModelIn,DualLayerModelEx,ModelType);}
    Compute_float* input=calloc(sizeof(Compute_float),grid_size*grid_size);
    Compute_float* input2=calloc(sizeof(Compute_float),grid_size*grid_size);
    randinit(input,OneLayerModel.potential); //need to fix for dual layer
    while (mytime<1000)
    {
        step_(input,input2);//always fine to pass an extra argument here
        printf("%i\n",mytime);
        for (int i=0;i<grid_size;i++)
        {
            for (int j=0;j<grid_size;j++)
            {
                input[i*grid_size+j]=m->layer1.voltages_out[i*grid_size + j];
                if (ModelType==DUALLAYER) { input2[i*grid_size+j]=m->layer2.voltages_out[i*grid_size + j];}
            }
        }
    }
    free(input);
    return(EXIT_SUCCESS);
}
#endif
