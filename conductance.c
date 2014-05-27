/// \file
#include <string.h> //memcpy
#include <getopt.h> //getopt
#include "STDP.h"
#include "evolve.h"
#include "newparam.h"
#include "init.h"
#include "yossarian.h"
#include "output.h"
unsigned int mytime=0;  ///<< The current time step
model* m;               ///< The model we are evolving through time
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
    memcpy(m->layer1.voltages,inp,sizeof(Compute_float)*grid_size*grid_size);
    if (ModelType==DUALLAYER) {memcpy(m->layer2.voltages,inp,sizeof(Compute_float)*grid_size*grid_size);}
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
        if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType,&Outputtable);} //pass the same layer as a double parameter
        else {m=setup(DualLayerModelEx,DualLayerModelIn,ModelType,&Outputtable);}
        setup_done=1;
        printf("setup done\n");
    }
    //error checking
    if (nrhs!=nlhs) {printf("We need the same number of parameters on the left and right hand side\n");return;}
    if (nrhs < 2)   {printf("At least input the voltage for both neuron populations\n");return;}
    if (mxGetClassID(prhs[0]) != MatlabDataType()) {printf("rhs parameter 1 is not of the correct data type (single/double)\n");return;}
    if (mxGetClassID(prhs[1]) != MatlabDataType()) {printf("rhs parameter 2 is not of the correct data type (single/double)\n");return;}
    if (mxGetM(prhs[0]) != grid_size || mxGetN(prhs[0]) != grid_size || mxGetNumberOfDimensions(prhs[0]) != 2) {printf("rhs parameter 1 has the wrong shape\n");return;}
    if (mxGetM(prhs[1]) != grid_size || mxGetN(prhs[1]) != grid_size || mxGetNumberOfDimensions(prhs[1]) != 2) {printf("rhs parameter 2 has the wrong shape\n");return;}
    for (int i = 2;i<nrhs;i++)
    {
        if (mxGetClassID(prhs[i]) != mxCHAR_CLASS) {printf("rhs parameter %i needs to be a char string\n");return;}
    }
        
    const Compute_float* inputdata = (Compute_float*) mxGetData(prhs[0]);
    const Compute_float* inputdata2 = (Compute_float*) mxGetData(prhs[1]);
    step_(inputdata,inputdata2); //always output the voltage data
    plhs[0]=mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
    plhs[1]=mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
    Compute_float* pointer1 = mxCalloc(grid_size*grid_size,sizeof(Compute_float));
    Compute_float* pointer2 = mxCalloc(grid_size*grid_size,sizeof(Compute_float));
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            pointer1[i*grid_size+j]=m->layer1.voltages_out[i*grid_size + j];
            pointer2[i*grid_size+j]=m->layer2.voltages_out[i*grid_size + j];
        }
    }
    mxSetData(plhs[0],pointer1);
    mxSetData(plhs[1],pointer2);
    if (nrhs>1) //output other stuff
    {
        int rhsidx = 2;
        while (rhsidx<nrhs)
        {
            char* data=malloc(sizeof(char)*1024); //should be big enough
            mxGetString(prhs[rhsidx],data,1023);
            int outidx = 0;
            int worked = 0;
            while (strlen(Outputtable[outidx].name) != 0)
            {
                if (!strcmp(Outputtable[outidx].name,data))
                {
                    plhs[rhsidx]=outputToMxArray(Outputtable[outidx].data);
                    worked = 1;
                    break;
                }
                outidx++;
            } 
            if (worked != 1) {printf("Unknown thing to output\n");return;}
            free(data);
            rhsidx++;
        }
    }
    return;
}
#else
///Runs some simple tests - currently broken
void tests()
{
    testmodparam(OneLayerModel);
    printf("tests passed");
}
///Structure which holds the command line options that the program recognises
struct option long_options[] = {{"help",no_argument,0,'h'},{"generate",no_argument,0,'g'},{"sweep",required_argument,0,'s'}};
///Main function for the entire program
/// @param argc number of cmdline args
/// @param argv what the parameters actually are
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
    if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType,&Outputtable);} //pass the same layer as a double parameter
    else {m=setup(DualLayerModelIn,DualLayerModelEx,ModelType,&Outputtable);}
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
    free(input2);
    return(EXIT_SUCCESS);
}
#endif
