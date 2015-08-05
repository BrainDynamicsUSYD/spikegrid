//#include "opencv2/contrib/contrib.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdbool>
#include "imread.h"
extern "C"
{
    //TODO: these includes have no business being in this file - need to restructure the dependencies here
#include "../randconns.h"
#include "../mymath.h"
#include "../STDP.h"
#include "../sizes.h"
}
typedef enum {Normal=0,Testing=1} Net_state;
bool cached=false;
Net_state state = Normal;
//I think that these global cv::mats are causing the pure virtual function problem
cv::Mat imcache; //elements have type Vec3b
cv::Mat ReadImage(const char* const path)
{
    cv::Mat m =cv::imread(path); //the pixels are stored with type cv::Vec3b - third element is red.  0 is black, 255 is white?
    return m;
}
bool fire1 = false;
bool fire2 = false;
void ResetVoltages(Compute_float* voltsin)
{
    for (int i=0;i<grid_size*grid_size;i++)
    {
        voltsin[i]=-100; //TODO - make this a parameter;
    }
}
void StartTesting(Compute_float* voltsin, STDP_data* S)
{

    std::cout << "Started testing" << std::endl;
    ResetVoltages(voltsin);
    state = Testing;
    fire1=false;
    fire2=false;
    if (S != NULL) //need to check if STDP points to NULL - which will occur if STDP is off
    {
        S->RecordSpikes = OFF;
    }
}
int onesfire=0;
int lastonesfire=-1;
void EndTesting(STDP_data* S, const int trialno,const Stimulus_parameters Stim)
{
    state = Normal;
    if (fire1 && fire2)
    {
        std::cout << "Both spiked: " << trialno << " " << onesfire << std::endl ; //endtesting actually gets called just after the start of the next trial - but this is good as it corrects for an off by one error that would otherwise occur as trials start at 0.
        exit(0);
    }
    if (S != NULL)
    {
        S->RecordSpikes = ON;
    }
}
bool path1;
bool path2; //make compiler happy - need to redo this whole function anyway
int  counts1;
Compute_float lastset;
bool stim1choice=true;
void CreateStims(const Compute_float timemodper,const Stimulus_parameters S,const Compute_float itercount)
{
    printf("createstims\n");
    stim1choice = RandFloat() > S.NoUSprob;
    if (stim1choice && (int)itercount > lastonesfire) {onesfire++;lastonesfire=(int)itercount; printf("increase\n");}
}
//TODO: this function is getting ridiculously long - needs to be tidied up.
//TODO: above note still true, adding extra parameters anyway
void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float threshold, STDP_data* stdp,const randconns_info* const rcinfo)
{
    if (cached==false) {imcache=ReadImage(S.ImagePath);cached=true;}
    const Compute_float timemodper = fmod(timemillis,S.timeperiod);
    const Compute_float itercount = timemillis/S.timeperiod;
    if (itercount < 1.0  && S.TestPathChoice) {return;} //do nothing in first period- in test path mode the first period fails so skip over it - is pretty short anyway as no testing

    if (S.TestPathChoice)
    {
        if (timemodper < 0.001 && fabs(timemillis - lastset) > 0.01)
        {
            lastset=timemillis;
            if (S.Oscillating_path==ON)
            {
                if (fmod(itercount/S.path_osc_freq,2)<1)  //are we on left / right branch
                {
                    path1=true;
                    path2=false;
                }
                else
                {
                    path1=false;
                    path2=true;
                }
                if (fmod(itercount,2)< 1)
                {
                    path1 = false;
                    path2 = false;
                    fire1 = false;
                    fire2 = false;
                    stdp->RecordSpikes = OFF;
                }
                if (fmod(itercount,2)==1)
                {
                    printf ("Res: %i %i\n",fire1,fire2);
                    stdp->RecordSpikes = ON;
                }
            }
            else
            {
                if (S.LotsofTesting==OFF)
                {
                    path1 = (RandFloat() < S.Prob1) && itercount < 21;
                    counts1 += path1==true?1:0;
                    if ((int)itercount==21) {fire1=false;fire2=false;}
                    if ((int)itercount==22) {printf("%i %i %i\n",counts1,fire1,fire2);exit(EXIT_SUCCESS);}
                    path2 = !path1 && itercount < 21;
                }
                else
                {
                    if ((int)itercount % 2 == 0)
                    {
                        //run a test - i.e. do nothing
                        path1=false;
                        path2=false;
                        fire1=false;
                        fire2=false;
                        //but we still want to activate the stimulus somehow - where do I do that? - later on - if both false, path2 rand stim will be activated
                    }
                    else
                    {
                        //so we ran a test on the previous trial - get result
                        printf("RESULT - %i %i %i\n",(int)itercount,fire1,fire2);
                        //now pick where we simulate
                        path1 = (RandFloat() < S.Prob1);
                        path2 = !path1;

                    }
                }
            }
        }
    }
    else
    {
        if (timemodper < 0.001 && fabs(timemillis - lastset) > 0.01)
        {
            lastset=timemillis;
            printf("picking stimulus\n");
            CreateStims(timemodper,S,itercount);
        }

    }
    if (S.Oscillating_path==ON) {if (path1==false && path2==false) {stdp->RecordSpikes=OFF;} else {stdp->RecordSpikes=ON;}}
    const bool stim1 = ((fabs(timemodper-80.0)<.01 && stim1choice) && itercount >= S.PreconditioningTrials)  ;  //late wave
    const bool stim2 =  fabs(timemodper-80.0 + S.lag)<.01  || fabs (timemodper-220 - 5 )<.01; //early wave - issues twice - first is normal, second is test trial.

    if (S.Testing == ON)
    {
        if (fabs(timemodper - 220) < 5) {StartTesting(voltsin,stdp);  }
        if (fabs(timemodper ) < 0.01) {EndTesting(stdp,(int)(itercount - S.PreconditioningTrials),S);  }
    }
    if (timemodper < 5) { ResetVoltages(voltsin);} //reset before next period.

    for (Neuron_coord x=0;x<grid_size;x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {
            const coords c = {.x=x,.y=y};
            const size_t idx = grid_index(c);
            //--------------
            //NOTE: opencv is completely nuts.  The returned order of color channels is BGR (NOT RGB)
            //--------------
            cv::Vec3b pixel = imcache.at<cv::Vec3b>(x,y);
            //uncomment to print colours - not particularly helpful
            //std::cout << x << "," << y << " " <<  pixel << std::endl;
            if (pixel == cv::Vec3b(0,0,0))
            {
                voltsin[idx]=-100;
            }
            else if ( stim1 && pixel == cv::Vec3b(0,0,255))
            {
                voltsin[idx]=100;
            }
            else if (stim2 && pixel == cv::Vec3b(255,0,0))
            {
                voltsin[idx]=100;
            }
            else if (path1 && pixel == cv::Vec3b(0,255,0))
            {
                voltsin[idx] = -100;
            }
            else if (path2 && pixel == cv::Vec3b(0,100,0))
            {
                voltsin[idx] = -100;
            }
            //detection loop - keep in a separate statement for now
            if (pixel == cv::Vec3b(100,100,100))
            {
                if (voltsin[idx] > threshold )
                {
                    fire1 = true;
                }
            }
            else if (pixel == cv::Vec3b(150,150,150))
            {
                if (voltsin[idx] > threshold )
                {
                    fire2 = true;
                }
            }
            else if (pixel == cv::Vec3b(50,50,50))
            {
                if (voltsin[idx] > threshold)
                {
                    //if both set to false, this will activate stimulus 2 for testing
                    if (path1) {voltsin[rcinfo->SpecialAInd]=100; }
                    else       {voltsin[rcinfo->SpecialBInd]=100;}
                }
            }
            else if (pixel == cv::Vec3b(0,255,255)) //yellow
            {
            }
            else if (pixel == cv::Vec3b(255,0,255)) //purple
            {
            }
        }
    }
}

void ApplyContinuousStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float Timestep,const Compute_float* Phimat)
{
    if (cached==false) {imcache=ReadImage(S.ImagePath);cached=true;}
    const Compute_float I0 = S.I0*Timestep;
    const Compute_float I1 = S.I1*Timestep;
    const Compute_float I2 = S.I2*Timestep;
    for (Neuron_coord x=0;x<grid_size;x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {
            const size_t idx = grid_index((coords){.x=x,.y=y});
            cv::Vec3b pixel = imcache.at<cv::Vec3b>(x,y);
            //uncomment to print colours - not particularly helpful
            //std::cout << x << "," << y << " " <<  pixel << std::endl;
            //constant external input - only makes sense for Euler method
            if (pixel == cv::Vec3b(0,106,127)) //0,127,127: background (olive - 128,128,0 in GIMP)
            {
                voltsin[idx] += I0;
                voltsin[idx] += I1*cos(2*M_PI*S.mu*timemillis+Phimat[idx]); //Need to code mu and phi
            }
            else if (pixel == cv::Vec3b(127,0,87)) //127,0,127:foreground (purple - 128,0,128 in GIMP)
            {
                voltsin[idx] += I2;
            }
        }
    }
}







