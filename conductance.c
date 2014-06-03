/// \file
//for debugging
#define _GNU_SOURCE
#include <string.h> //memcpy
#include <getopt.h> //getopt
#include <stdlib.h> //exit
#include <fenv.h>   //for some debugging
#include "STDP.h"
#include "evolve.h"
#include "newparam.h"
#include "init.h"
#include "yossarian.h"
#include "matlab_includes.h"
#include "matlab_output.h"
unsigned int mytime=0;  ///<< The current time step
model* m;               ///< The model we are evolving through time
int jobnumber=-1;        ///< The current job number - used for pics directory etc
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
}
#ifdef MATLAB
int setup_done=0;
mxArray* CreateInitialVoltage(conductance_parameters c)
{
    mxArray* volts =mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
    Compute_float* voltdata = (Compute_float*)mxGetData(volts);
    randinit(voltdata,c);
    return volts;
}

mxArray* FirstMatlabCall( )
{
 //   feenableexcept(FE_INVALID | FE_OVERFLOW);
    if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType,jobnumber);} //pass the same layer as a double parameter
    else {m=setup(DualLayerModelEx,DualLayerModelIn,ModelType,jobnumber);}
    //set up initial voltage matrix - we need a different number if we are in single or double layer model - so encase the voltages in a struct
    mxArray* voltages = mxCreateStructMatrix(1,1,3,(const char*[]){"Vin","Vex","Vsingle_layer"});
    if (ModelType==SINGLELAYER)
    {
        mxSetField(voltages,0,"Vsingle_layer",CreateInitialVoltage(OneLayerModel.potential));
    }
    else
    {
        mxSetField(voltages,0,"Vin",CreateInitialVoltage(DualLayerModelIn.potential));
        mxSetField(voltages,0,"Vex",CreateInitialVoltage(DualLayerModelEx.potential));
    }
    ///Now - do some dummy outputs of the other elements so that the graphs can be set up.
    printf("setup done\n");
    return voltages;
}
///Matlab entry point. The pointers give access to the left and right hand sides of the function call.
///Note that it is required to assign a value to all entries on the left hand side of the equation.
///Dailing to do so will produce an error in matlab.
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
    if (nrhs!=nlhs) {printf("We need the same number of parameters on the left and right hand side\n");return;}
    if (setup_done==0) 
    {
        plhs[0]=FirstMatlabCall();
        outputExtraThings(plhs,nrhs,prhs);
        setup_done=1;
        return;
    }
    //error checking
   // if (mxGetClassID(prhs[0]) != MatlabDataType()) {printf("rhs parameter 1 is not of the correct data type (single/double)\n");return;}
  //  if (mxGetClassID(prhs[1]) != MatlabDataType()) {printf("rhs parameter 2 is not of the correct data type (single/double)\n");return;}
  //  if (mxGetM(prhs[0]) != grid_size || mxGetN(prhs[0]) != grid_size || mxGetNumberOfDimensions(prhs[0]) != 2) {printf("rhs parameter 1 has the wrong shape\n");return;}
  //  if (mxGetM(prhs[1]) != grid_size || mxGetN(prhs[1]) != grid_size || mxGetNumberOfDimensions(prhs[1]) != 2) {printf("rhs parameter 2 has the wrong shape\n");return;}
    //step the model through time
    mxArray* voltages = mxCreateStructMatrix(1,1,3,(const char*[]){"Vin","Vex","Vsingle_layer"});
    if (ModelType == SINGLELAYER)
    {
        step_((Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vsingle_layer")),NULL /*no second layer*/);
        mxArray* out = mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
        memcpy((Compute_float*)mxGetData(out),m->layer1.voltages_out,sizeof(Compute_float)*grid_size*grid_size);
        mxSetField(voltages,0,"Vsingle_layer",out);
    }
    else
    {
        step_((Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vex")),
                (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vin")));
        mxArray* out1 = mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
        mxArray* out2 = mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
        memcpy((Compute_float*)mxGetData(out1),m->layer1.voltages_out,sizeof(Compute_float)*grid_size*grid_size);
        memcpy((Compute_float*)mxGetData(out2),m->layer2.voltages_out,sizeof(Compute_float)*grid_size*grid_size);
        mxSetField(voltages,0,"Vex",out1);
        mxSetField(voltages,0,"Vin",out2);
    }
    plhs[0] = voltages;
    outputExtraThings(plhs,nrhs,prhs);
    return;
}
#else
///Structure which holds the command line options that the program recognises
struct option long_options[] = {{"help",no_argument,0,'h'},{"generate",no_argument,0,'g'},{"sweep",required_argument,0,'s'}};
///Main function for the entire program
/// @param argc number of cmdline args
/// @param argv what the parameters actually are
int main(int argc,char** argv) //useful for testing w/out matlab
{
    parameters* newparam = NULL;
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
                    jobnumber=atoi(optarg);
                    printf("doing sweep index %i\n",jobnumber);
                    if (ModelType == SINGLELAYER)
                    {
                           newparam = GetNthParam(OneLayerModel,Sweep,(unsigned int)jobnumber);
                    }
                    else 
                    {
                           newparam = GetNthParam(DualLayerModelEx,Sweep,(unsigned int)jobnumber);
                    }
                }
                break;
        }
    }
    if (ModelType==SINGLELAYER) {m=setup(newparam!=NULL? (*newparam):OneLayerModel,newparam!=NULL? (*newparam):OneLayerModel,ModelType,jobnumber);} //pass the same layer as a double parameter
    else {m=setup(DualLayerModelIn,newparam!=NULL?*newparam:DualLayerModelEx,ModelType,jobnumber);}
    Compute_float* input=calloc(sizeof(Compute_float),grid_size*grid_size);
    Compute_float* input2=calloc(sizeof(Compute_float),grid_size*grid_size);
    randinit(input,DualLayerModelEx.potential); //need to fix for single layer
    randinit(input2,DualLayerModelIn.potential); 
    while (mytime<Features.Simlength)
    {
        step_(input2,input);//always fine to pass an extra argument here
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
