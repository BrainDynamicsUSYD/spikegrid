/// \file
//for debugging
#define _GNU_SOURCE
#include <string.h> //memcpy
#include <getopt.h> //getopt
#include <stdlib.h> //exit
#include <fenv.h>   //for some debugging
#include <stdio.h>
#include <time.h>
#include "gui.h"
#include "cleanup.h"
#include "evolve.h"
#include "newparam.h"
#include "init.h"
#include "yossarian.h"
#include "paramheader.h"
#include "model.h"
#include "out/out.h"
#include "openCVAPI/api.h"
#ifdef ANDROID
    #define APPNAME "myapp"
    #include <android/log.h>
#endif

unsigned int mytime=0;  ///<< The current time step
model* m;               ///< The model we are evolving through time
int jobnumber=-1;        ///< The current job number - used for pics directory etc
int yossarianjobnumber=-1;
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
        if(inpV2==NULL)                         {printf("missing second input voltage in dual-layer model"); exit(EXIT_FAILURE);}
        if(Features.Recovery==ON && inpW2==NULL){printf("missing second recovery input in dual-layer model");exit(EXIT_FAILURE);}
    }
    mytime++;
    memcpy(m->layer1.voltages,inpV,sizeof(Compute_float)*grid_size*grid_size);
    if (Features.Recovery==ON) {memcpy(m->layer1.recoverys,inpW,sizeof(Compute_float)*grid_size*grid_size);}
    if (ModelType==DUALLAYER)
    {
        memcpy(m->layer2.voltages,inpV2,sizeof(Compute_float)*grid_size*grid_size);
        if (Features.Recovery==ON) {memcpy(m->layer2.recoverys,inpW2,sizeof(Compute_float)*grid_size*grid_size);}
    }
    step1(m,mytime);
    DoOutputs(mytime);
}

//I am not a huge fan of this function.  A nicer version would be good.
void setuppointers(Compute_float** FirstV,Compute_float** SecondV, Compute_float** FirstW, Compute_float** SecondW,const Job* const job)
{
    *FirstV = calloc(sizeof(Compute_float),grid_size*grid_size); //we always need the voltage in the first layer
    if (ModelType==SINGLELAYER)
    {
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
            *FirstW  = calloc(sizeof(Compute_float),grid_size*grid_size);
            *SecondW = calloc(sizeof(Compute_float),grid_size*grid_size);
        } else {*FirstW=NULL;*SecondW=NULL;}
    }
}

#ifdef MATLAB
//The easeiest way to get data out with matlab is to use outputtomxarray and the easiest way to use that is with getoutputbyname.  Getoutputbyname is in output.h so include it.
//#include "matlab_output.h"
int setup_done=0;
mxArray* CreateInitialValues(const Compute_float minval, const Compute_float maxval)
{
    mxArray* vals =mxCreateNumericMatrix(grid_size,grid_size,MatlabDataType(),mxREAL);
    Compute_float* datavals = (Compute_float*)mxGetData(vals);
    randinit(datavals,minval,maxval);
    return vals;
}
typedef struct mexmap
{
    const char* const name;
    const char* outname;
    Compute_float** data;
    const LayerNumbers Lno;
    const on_off recovery;
    Compute_float init_min;
    Compute_float init_max;
} mexmap;

///Matlab entry point. The pointers give access to the left and right hand sides of the function call.
///Note that it is required to assign a value to all entries on the left hand side of the equation.
///Dailing to do so will produce an error in matlab.
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
    if (nrhs!=nlhs) {printf("We need the same number of parameters on the left and right hand side\n");return;}
    Compute_float *FirstV,*SecondV,*FirstW,*SecondW;
    mexmap mexmappings[] = {
        //name in matlab , outputtable , pointer  , when which layer , is recovery variable , initial min value              , initial max value
        {"Vsingle_layer" , "V1"        , &FirstV  , SINGLELAYER      , OFF                  , OneLayerModel.potential.Vrt    , OneLayerModel.potential.Vpk}    ,
        {"Wsingle_layer" , "W1"        , &FirstW  , SINGLELAYER      , ON                   , Zero                           , Zero}                           ,
        {"Vin"           , "V1"        , &FirstV  , DUALLAYER        , OFF                  , DualLayerModelIn.potential.Vrt , DualLayerModelIn.potential.Vpk} ,
        {"Vex"           , "V2"        , &SecondV , DUALLAYER        , OFF                  , DualLayerModelEx.potential.Vrt , DualLayerModelEx.potential.Vpk} ,
        {"Win"           , "Recovery1" , &FirstW  , DUALLAYER        , ON                   , Zero                           , Zero}                           ,
        {"Wex"           , "Recovery2" , &SecondW , DUALLAYER        , ON                   , Zero                           , Zero}                           ,
        {0               , 0           , 0        , 0                , 0                    , 0                              , 0}};
    //the entries in this array are the first column of the previous array
    mxArray* variables = mxCreateStructMatrix(1,1,6,(const char*[]){"Vin","Vex","Win","Wex","Vsingle_layer","Wsingle_layer"});
    if (setup_done==0)
    {
        srandom((unsigned)time(0));
        if (ModelType==SINGLELAYER) {m=setup(OneLayerModel,OneLayerModel,ModelType,jobnumber,yossarianjobnumber);} //pass the same layer as a double parameter
        else {m=setup(DualLayerModelIn,DualLayerModelEx,ModelType,jobnumber,yossarianjobnumber);}
        int i = 0;
        while(mexmappings[i].name != NULL)
        {
            if (ModelType==mexmappings[i].Lno && (Features.Recovery==ON || Features.Recovery==mexmappings[i].recovery))
            {
                mxSetField(variables,0,mexmappings[i].name,CreateInitialValues(mexmappings[i].init_min,mexmappings[i].init_max));
            }
            i++;
        }
        setup_done=1;
    }
    else
    {
        //step the model through time
        //First - get inputs
        int i = 0;
        while(mexmappings[i].name != NULL)
        {
            if (ModelType==mexmappings[i].Lno && (Features.Recovery==ON || Features.Recovery==mexmappings[i].recovery))
            {
                (*mexmappings[i].data) =(Compute_float* ) mxGetData(mxGetField(prhs[0],0,mexmappings[i].name));
            }
            i++;
        }
        //Actually step the model
        step_(FirstV,SecondV,FirstW,SecondW);
        //set outputs
        i=0;
        while(mexmappings[i].name != NULL)
        {
            if (ModelType==mexmappings[i].Lno && (Features.Recovery==ON || Features.Recovery==mexmappings[i].recovery))
            {
                mxSetField(variables,0,mexmappings[i].name,outputToMxArray(getOutputByName(mexmappings[i].outname)));
            }
            i++;
        }
    }
    plhs[0] = variables;
    outputExtraThings(plhs,nrhs,prhs);
    return;
}
#else
///Structure which holds the command line options that the program recognises
struct option long_options[] = {{"help",no_argument,0,'h'},{"generate",no_argument,0,'g'},{"sweep",required_argument,0,'s'},{"nocv",no_argument,0,'n'},{"nosegfault",no_argument,0,'f'},{0,0,0,0}};
void processopts (int argc,char** argv,parameters** newparam,parameters** newparamEx,parameters** newparamIn,on_off* OpenCv)
{
    while (1)
    {
        int option_index=0;
        int c=getopt_long(argc,argv,"hgns:f",long_options,&option_index);
        if (c==-1) {break;} //end of options
        switch (c)
        {
            case 'h':
                printf("available arguments:\n"
                        "   -h --help print this message\n"
                        "   -g --generate generate a yossarian config file\n"
                        "   -n --nocv disable open cv viewer\n"
                        "   -s --sweep N do the nth element of a sweep\n"
                        "   -f --nosegfault prevents almost all segfaults");
                exit(EXIT_SUCCESS);
            case 'g':
                createyossarianfile("yossarian.csh",Sweep);
                exit(EXIT_SUCCESS);
            case 's':
                {
                    yossarianjobnumber=atoi(optarg);
                    printf("doing sweep index %i\n",yossarianjobnumber);
                    if (ModelType == SINGLELAYER)
                    {
                        *newparam = GetNthParam(OneLayerModel,Sweep,(unsigned int)yossarianjobnumber);
                    }
                    else
                    {
                        *newparamEx =Sweep.SweepEx==ON?GetNthParam(DualLayerModelEx,Sweep,(unsigned int)yossarianjobnumber):NULL;
                        *newparamIn =Sweep.SweepIn==ON?GetNthParam(DualLayerModelIn,Sweep,(unsigned int)yossarianjobnumber):NULL;
                    }
                }
                break;
            case 'n':
                *OpenCv = OFF;
                break;
            case 'f':
                exit(EXIT_SUCCESS);
        }
    }
}
///Main function for the entire program
/// @param argc number of cmdline args
/// @param argv what the parameters actually are
int main(int argc,char** argv) //useful for testing w/out matlab
{
    const char* CVDisplay[] = {"gE","V2","SV","STDP1","STDP2"}; //list of possible variables to show
    const int CVNumWindows=2;                              //and how many to show
#ifndef ANDROID //android doesn't support this function - note the error is that this will fail at linking so it needs to hide in the #if
 //   feenableexcept(FE_INVALID | FE_OVERFLOW); //segfault on NaN and overflow.  Note - this cannot be used in matlab
#endif
    parameters* newparam = NULL;
    parameters* newparamEx = NULL;
    parameters* newparamIn = NULL;
    setvbuf(stdout,NULL,_IONBF,0);
    on_off OpenCv=ON;
    processopts(argc,argv,&newparam,&newparamEx,&newparamIn,&OpenCv);

    const Job* job = &Features.job;
    if (job->next != NULL || (job->initcond==RAND_JOB && job->Voltage_or_count>1)) {jobnumber=0;} //if more than one job - then start at 0 - so that stuff goes in folders
    while (job != NULL)
    {
        int count = job->initcond==RAND_JOB?(int)job->Voltage_or_count:1; //default to 1 job
        for (int c = 0;c<count;c++)
        {
            mytime=0;
            //seed RNG as appropriate - with either time or job number
            if     (job->initcond == RAND_TIME){srandom((unsigned)time(0));}
            else if(job->initcond==RAND_JOB)   {srandom((unsigned)c);}
            else if(job->initcond==RAND_ZERO)  {srandom((unsigned)0);}
            //sets up the model code
            if (ModelType==SINGLELAYER) {m=setup(newparam!=NULL? (*newparam):OneLayerModel,newparam!=NULL? (*newparam):OneLayerModel,ModelType,jobnumber,yossarianjobnumber);} //pass the same layer as a double parameter
            else {m=setup(newparamIn!=NULL?*newparamIn:DualLayerModelIn,newparamEx!=NULL?*newparamEx:DualLayerModelEx,ModelType,jobnumber,yossarianjobnumber);}

#ifdef OPENCV
            if (OpenCv==ON){ cvdispInit(CVDisplay,CVNumWindows);}
#endif
            Compute_float *FirstV,*SecondV,*FirstW,*SecondW;
            setuppointers(&FirstV,&SecondV,&FirstW,&SecondW,job);
            //actually runs the model
            while (mytime<Features.Simlength)
            {

                if (mytime%10==0){printf("%i\n",mytime);}
                step_(FirstV,SecondV,FirstW,SecondW);//always fine to pass an extra argument here
                //copy the output to be new input
                memcpy ( FirstV, m->layer1.voltages_out, sizeof ( Compute_float)*grid_size*grid_size);
                if(SecondV != NULL){memcpy(SecondV,m->layer2.voltages_out, sizeof(Compute_float)*grid_size*grid_size);}
                if(FirstW != NULL) {memcpy(FirstW, m->layer1.recoverys_out,sizeof(Compute_float)*grid_size*grid_size);}
                if(SecondW != NULL){memcpy(SecondW,m->layer2.recoverys_out,sizeof(Compute_float)*grid_size*grid_size);}
                //do some opencv stuff
#ifdef OPENCV
                if(mytime % 40 ==0 && OpenCv == ON)
                {
                    cvdisp(CVDisplay,CVNumWindows,m->layer2.rcinfo,m->layer2.STDP_data);
                }
#endif
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
#ifdef ANDROID
#include <jni.h>
int android_setup_done=0;
Compute_float *FirstV,*SecondV,*FirstW,*SecondW;
//nice riduclously long function name
JNIEXPORT jdoubleArray JNICALL Java_com_example_conductanceandroid_MainActivity_AndroidEntry(JNIEnv* env, jobject jboj)
{
    if (android_setup_done==0)
    {
        android_setup_done=1;
        __android_log_print(ANDROID_LOG_VERBOSE,APPNAME,"starting code");
        const Job* job = &Features.job;
        srandom((unsigned)time(0));
        //sets up the model code
        m=setup(DualLayerModelIn,DualLayerModelEx,ModelType,0);
        setuppointers(&FirstV,&SecondV,&FirstW,&SecondW,job);
        __android_log_print(ANDROID_LOG_VERBOSE,APPNAME,"setup done");
    }
    //actually runs the model
    for (int i=0;i<5;i++)
    {
        step_(FirstV,SecondV,FirstW,SecondW);//always fine to pass an extra argument here
        //copy the output to be new input
        memcpy ( FirstV, m->layer1.voltages_out, sizeof ( Compute_float)*grid_size*grid_size);
        if(SecondV != NULL){memcpy(SecondV,m->layer2.voltages_out, sizeof(Compute_float)*grid_size*grid_size);}
        if(FirstW != NULL) {memcpy(FirstW, m->layer1.recoverys_out,sizeof(Compute_float)*grid_size*grid_size);}
        if(SecondW != NULL){memcpy(SecondW,m->layer2.recoverys_out,sizeof(Compute_float)*grid_size*grid_size);}
    }
    jdoubleArray ret = (*env) ->NewDoubleArray(env,grid_size*grid_size);
    (*env)->SetDoubleArrayRegion(env, ret, 0, grid_size*grid_size, SecondV );
    return ret;
}
#endif
#endif
