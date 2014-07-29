/// \file
//for debugging
#define _GNU_SOURCE
#include <string.h> //memcpy
#include <getopt.h> //getopt
#include <stdlib.h> //exit
#include <fenv.h>   //for some debugging
#include <stdio.h>
#include <time.h>
#include "cleanup.h"
#include "evolve.h"
#include "newparam.h"
#include "init.h"
#include "yossarian.h"
#include "matlab_output.h"
#include "paramheader.h"
#include "layer.h"
unsigned int mytime=0;  ///<< The current time step
model* m;               ///< The model we are evolving through time
int jobnumber=-1;        ///< The current job number - used for pics directory etc
//DO NOT CALL THIS FUNCTION "step" - this causes a weird collision in matlab that results in segfaults.  Incredibly fun to debug
///Function that steps the model through time (high level).
/// @param inpV the input voltages
/// @param inpV2 input voltages for layer 2.  In the single layer model a dummy argument needs to be passed.
/// @param inpW the input recoveries
/// @param inpW2 input recoveries for layer 2.  In the single layer model a dummy argument needs to be passed.
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
   // ringbuffer_increment(&m->layer1.spikes,mytime);
   // if (Features.STDP==ON) {ringbuffer_increment(&m->layer1.spikes_STDP,mytime);}
    if (ModelType==DUALLAYER) 
    {
        memcpy(m->layer2.voltages,inpV2,sizeof(Compute_float)*grid_size*grid_size);
        if (Features.Recovery==ON) {memcpy(m->layer2.recoverys,inpW,sizeof(Compute_float)*grid_size*grid_size);}
     //   ringbuffer_increment(&m->layer2.spikes,mytime);
       // if (Features.STDP==ON) {ringbuffer_increment(&m->layer2.spikes_STDP,mytime);}
    }
    step1(m,mytime);

}

void setuppointers(Compute_float** FirstV,Compute_float** SecondV, Compute_float** FirstW, Compute_float** SecondW,const Job* const job)
{
    if (ModelType==SINGLELAYER)
    {
        *FirstV = calloc(sizeof(Compute_float),grid_size*grid_size);
        *SecondV = NULL;
        if (job->initcond == SINGLE_SPIKE) {Fixedinit(*FirstV,OneLayerModel.potential.Vrt,job->Voltage_or_count);}
        else                               {randinit(*FirstV,OneLayerModel.potential.Vrt,OneLayerModel.potential.Vpk);}
        if (Features.Recovery==ON)
        {
            *FirstW = calloc(sizeof(Compute_float),grid_size*grid_size);
            *SecondW = NULL;
        } else {*FirstW=NULL;*SecondW=NULL;}
    }
    else if (ModelType==DUALLAYER) 
    {
        *FirstV = malloc(sizeof(Compute_float)*grid_size*grid_size);
        *SecondV = malloc(sizeof(Compute_float)*grid_size*grid_size);
        if (job->initcond == SINGLE_SPIKE) 
        {
            Fixedinit(*FirstV, DualLayerModelIn.potential.Vrt,job->Voltage_or_count);
            Fixedinit(*SecondV,DualLayerModelEx.potential.Vrt,job->Voltage_or_count);
        }
        else
        {
            randinit(*FirstV, DualLayerModelIn.potential.Vrt,DualLayerModelIn.potential.Vpk);
            randinit(*SecondV,DualLayerModelEx.potential.Vrt,DualLayerModelEx.potential.Vpk);
        }
        if (Features.Recovery==ON) 
        {
            *FirstW =   calloc(sizeof(Compute_float),grid_size*grid_size);
            *SecondW = calloc(sizeof(Compute_float),grid_size*grid_size);
        } else {*FirstW=NULL;*SecondW=NULL;}
    }
}

#ifdef MATLAB
int setup_done=0;
mxArray* CreateInitialValues(const Compute_float minval, const Compute_float maxval)
{
    mxArray* vals =mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
    Compute_float* datavals = (Compute_float*)mxGetData(vals);
    randinit(datavals,minval,maxval);
    return vals;
}

mxArray* FirstMatlabCall( )
{
    srandom((unsigned)time(0));
    if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType,jobnumber);} //pass the same layer as a double parameter
    else {m=setup(DualLayerModelIn,DualLayerModelEx,ModelType,jobnumber);}
    //set up initial voltage matrix - we need a different number if we are in single or double layer model - so encase the voltages in a struct
    mxArray* variables = mxCreateStructMatrix(1,1,6,(const char*[]){"Vin","Vex","Win","Wex","Vsingle_layer","Wsingle_layer"});
    if (ModelType==SINGLELAYER)
    {
        mxSetField(variables,0,"Vsingle_layer",CreateInitialValues(OneLayerModel.potential.Vrt,OneLayerModel.potential.Vpk));
        if (Features.Recovery==ON)
            {mxSetField(variables,0,"Wsingle_layer",CreateInitialValues(Zero,Zero));}
    }
    else if (ModelType==DUALLAYER) 
    {
        mxSetField(variables,0,"Vin",CreateInitialValues(DualLayerModelIn.potential.Vrt,DualLayerModelIn.potential.Vpk));
        mxSetField(variables,0,"Vex",CreateInitialValues(DualLayerModelEx.potential.Vrt,DualLayerModelEx.potential.Vpk));
        if (Features.Recovery==ON) 
        {
            mxSetField(variables,0,"Win",CreateInitialValues(Zero,Zero));
            mxSetField(variables,0,"Wex",CreateInitialValues(Zero,Zero));
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
        outputExtraThings(plhs,nrhs,prhs); 
        setup_done=1;
        return;
    }
    //step the model through time
    mxArray* variables = mxCreateStructMatrix(1,1,6,(const char*[]){"Vin","Vex","Win","Wex","Vsingle_layer","Wsingle_layer"});
    Compute_float *FirstV,*SecondV,*FirstW,*SecondW;
    //First - get inputs
    if (ModelType==SINGLELAYER)
    {
        FirstV = (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vsingle_layer"));
        SecondV = NULL;
        if (Features.Recovery==ON)
        {
            FirstW = (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Wsingle_layer"));
            SecondW = NULL;
        } else {FirstW=NULL;SecondW=NULL;}
    }
    else
    {
        FirstV =  (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vin"));
        SecondV = (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Vex"));
        if (Features.Recovery==ON)
        {
            FirstW  = (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Win"));
            SecondW = (Compute_float*) mxGetData(mxGetField(prhs[0],0,"Wex"));
        } else {FirstW=NULL;SecondW=NULL;}
    }
    //Actually step the model
    step_(FirstV,SecondV,FirstW,SecondW);
    //Now assign the outputs
    if (ModelType == SINGLELAYER)
    {
        mxSetField(variables,0,"Vsingle_layer",outputToMxArray(getOutputByName("V1")));
        if (Features.Recovery == ON) 
        {
            mxSetField(variables,0,"Wsingle_layer",outputToMxArray(getOutputByName("Recovery1")));
        }
    }
    else
    {
        mxSetField(variables,0,"Vex",outputToMxArray(getOutputByName("V2")));
        mxSetField(variables,0,"Vin",outputToMxArray(getOutputByName("V1")));
        if (Features.Recovery == ON)
        {
            mxSetField(variables,0,"Wex",outputToMxArray(getOutputByName("Recovery2")));
            mxSetField(variables,0,"Win",outputToMxArray(getOutputByName("Recovery1")));
        }
    }
    plhs[0] = variables;
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
    feenableexcept(FE_INVALID | FE_OVERFLOW); //segfault on NaN and overflow.  Note - this cannot be used in matlab
    parameters* newparam = NULL;
    setvbuf(stdout,NULL,_IONBF,0);
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
    const Job* job = &Features.job;
    if (job->next != NULL || (job->initcond==RAND_JOB && job->Voltage_or_count>1)) {jobnumber=0;} //if more than one job - then start at 0 - so that stuff goes in folders
    while (job != NULL)
    {
        int count = job->initcond==RAND_JOB?(int)job->Voltage_or_count:1; //default to 1 job
        for (int c = 0;c<count;c++)
        {
            mytime=0;
            //seed RNG as appropriate - with either time or job number
            if (job->initcond == RAND_TIME) {srandom((unsigned)time(0));}
            else if (job->initcond==RAND_JOB) {srandom((unsigned)c);}
            //sets up the model code
            if (ModelType==SINGLELAYER) {m=setup(newparam!=NULL? (*newparam):OneLayerModel,newparam!=NULL? (*newparam):OneLayerModel,ModelType,jobnumber);} //pass the same layer as a double parameter
            else {m=setup(DualLayerModelIn,newparam!=NULL?*newparam:DualLayerModelEx,ModelType,jobnumber);}

            Compute_float *FirstV,*SecondV,*FirstW,*SecondW;
            setuppointers(&FirstV,&SecondV,&FirstW,&SecondW,job);
            //actually runs the model
            while (mytime<Features.Simlength)
            {

                if (mytime%10==0){printf("%i\n",mytime);}
                step_(FirstV,SecondV,FirstW,SecondW);//always fine to pass an extra argument here
                //copy the output to be new input
                memcpy                       (FirstV, m->layer1.voltages_out, sizeof(Compute_float)*grid_size*grid_size);
                if (SecondV != NULL)  {memcpy(SecondV,m->layer2.voltages_out, sizeof(Compute_float)*grid_size*grid_size);}
                if (FirstW != NULL)   {memcpy(FirstW, m->layer1.recoverys_out,sizeof(Compute_float)*grid_size*grid_size);}
                if (SecondW != NULL)  {memcpy(SecondW,m->layer2.recoverys_out,sizeof(Compute_float)*grid_size*grid_size);}
            }
            FreeIfNotNull(FirstV);
            FreeIfNotNull(SecondV);
            FreeIfNotNull(FirstW);
            FreeIfNotNull(SecondW);
            CleanupModel(m);
            jobnumber++;
        }
        job=job->next;
    }
    return(EXIT_SUCCESS);
}
#endif
