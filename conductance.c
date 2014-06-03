/// \file
//for debugging
#define _GNU_SOURCE
#include <string.h> //memcpy
#include <getopt.h> //getopt
#include <stdlib.h> //exit
#include <fenv.h>   //for some debugging
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
void step_(const Compute_float* const inpV,const Compute_float* const inpV2, const Compute_float* inpW, const Compute_float* inpW2)
{
    if (Features.Recovery==ON && inpW==NULL){printf("missing first recovery input");exit(EXIT_FAILURE);}
    if (ModelType==DUALLAYER)
    {
        if (inpV2==NULL){printf("missing second input voltage in dual-layer model");exit(EXIT_FAILURE);}
        if (Features.Recovery==ON && inpW2==NULL){printf("missing second recovery input in dual-layer model");exit(EXIT_FAILURE);}
    }       
    mytime++;
    memcpy(m->layer1.voltages,inpV,sizeof(Compute_float)*grid_size*grid_size);
    if (Features.Recovery==ON) {memcpy(m->layer1.recoverys,inpW,sizeof(Compute_float)*grid_size*grid_size);}
    m->layer1.spikes.curidx=mytime%(m->layer1.spikes.count);
    if (ModelType==DUALLAYER) 
    {
        memcpy(m->layer2.voltages,inpV,sizeof(Compute_float)*grid_size*grid_size);
        if (Features.Recovery==ON) {memcpy(m->layer2.recoverys,inpW,sizeof(Compute_float)*grid_size*grid_size);}
        m->layer2.spikes.curidx=mytime%(m->layer2.spikes.count);
    }
    step1(m,mytime);

}
#ifdef MATLAB
int setup_done=0;
//TODO: this should just take a min and a max value
mxArray* CreateInitialValues(const Compute_float minval, const Compute_float maxval)
{
    mxArray* vals =mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
    Compute_float* datavals = (Compute_float*)mxGetData(vals);
    randinit(datavals,minval,maxval);
    return vals;
}

mxArray* FirstMatlabCall( )
{
 //   feenableexcept(FE_INVALID | FE_OVERFLOW);
    if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType,jobnumber);} //pass the same layer as a double parameter
    else {m=setup(DualLayerModelEx,DualLayerModelIn,ModelType,jobnumber);}
    //set up initial voltage matrix - we need a different number if we are in single or double layer model - so encase the voltages in a struct
    mxArray* variables = mxCreateStructMatrix(1,1,3,(const char*[]){"Vin","Vex","Win","Wex","Vsingle_layer","Wsingle_layer"});
    if (ModelType==SINGLELAYER)
        {mxSetField(variables,0,"Vsingle_layer",CreateInitialValues(OneLayerModel.potential,1));}
    if (Features.Recovery==ON)
        {mxSetField(variables,0,"Wsingle_layer",CreateInitialValues(OneLayerModel.potential,0));}
    else if (ModelType==DUALLAYER) 
    {
        mxSetField(variables,0,"Vin",CreateInitialValues(DualLayerModelIn.potential,1));
        mxSetField(variables,0,"Vex",CreateInitialValues(DualLayerModelEx.potential,1));
        if (Features.Recovery==ON) 
        {
            mxSetField(variables,0,"Win",CreateInitialValues(DualLayerModelIn.potential,0));
            mxSetField(variables,0,"Wex",CreateInitialValues(DualLayerModelEx.potential,1));
        }
    }
    ///Now - do some dummy outputs of the other elements so that the graphs can be set up.
    printf("setup done\n");
    return variables;
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
        outputExtraThings(plhs,nrhs,prhs); //what is this and where does it come from?
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
        if (Features.Recovery==OFF) 
        {
            step_((Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vsingle_layer")),NULL,NULL,NULL);
            mxSetField(voltages,0,"Vsingle_layer",outputToMxArray((tagged_array){.data=m->layer1.voltages_out,.size=grid_size,.offset=0}));
        }
        else if (Features.Recovery==ON) 
        {
            step_((Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vsingle_layer")),NULL,
                (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Wsingle_layer")),NULL);
            mxSetField(voltages,0,"Vex",outputToMxArray((tagged_array){.data=m->layer1.voltages_out,.size=grid_size,.offset=0}));
            mxSetField(voltages,0,"Wex",outputToMxArray((tagged_array){.data=m->layer1.recoverys_out,.size=grid_size,.offset=0}));
        }
    }
    else if (ModelType == DUALLAYER) 
    {
        if (Features.Recovery==OFF)
        {
            step_((Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vex")),
                    (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vin")),
                    NULL,NULL);
            mxSetField(voltages,0,"Vex",outputToMxArray((tagged_array){.data=m->layer1.voltages_out,.size=grid_size,.offset=0}));
            mxSetField(voltages,0,"Vin",outputToMxArray((tagged_array){.data=m->layer2.voltages_out,.size=grid_size,.offset=0}));
        }
        else if (Features.Recovery==ON)
        {
            step_((Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vex")),
                    (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vin")),
                    (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Wex")),
                    (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Win")));
            //put data into matlab struct
            mxSetField(voltages,0,"Vex",outputToMxArray((tagged_array){.data=m->layer1.voltages_out,.size=grid_size,.offset=0}));
            mxSetField(voltages,0,"Vin",outputToMxArray((tagged_array){.data=m->layer2.voltages_out,.size=grid_size,.offset=0}));
            mxSetField(voltages,0,"Wex",outputToMxArray((tagged_array){.data=m->layer1.recoverys_out,.size=grid_size,.offset=0}));
            mxSetField(voltages,0,"Win",outputToMxArray((tagged_array){.data=m->layer2.recoverys_out,.size=grid_size,.offset=0}));
        }
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
    randinit(input,DualLayerModelEx.potential.Vrt,DualLayerModelEx.potential.Vrt+(1/20.0)); //need to fix for single layer
    randinit(input2,DualLayerModelIn.potential.Vrt,DualLayerModelIn.potential.Vrt+(1/20.0)); 
    while (mytime<Features.Simlength)
    {
        //step_(input2,input);//always fine to pass an extra argument here
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
