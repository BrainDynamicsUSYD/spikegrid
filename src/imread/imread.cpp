//#include "opencv2/contrib/contrib.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdbool>
#include <algorithm>
#include <random>
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
    if (m.data==NULL)
    {
        std::cout << "Tried to read image from " << path << " failed - exiting" << std::endl;
        exit(EXIT_FAILURE);
    }
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
void EndTesting(STDP_data* S, const int trialno)
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
void CreateStims(const Stimulus_parameters S,const Compute_float itercount)
{
    printf("createstims\n");
    stim1choice = RandFloat() > S.NoUSprob;
    if (stim1choice && (int)itercount > lastonesfire) {onesfire++;lastonesfire=(int)itercount; printf("increase\n");}
}

double storerand=0.3;
std::default_random_engine generator(1);
//std::bernoulli_distribution distribution(0.5);
std::normal_distribution<double> distribution(0,0.05);
double nextrand()
{
/*    bool newrand=distribution(generator);
    if (newrand==true)
    {
        storerand=storerand+0.02;
    }
    else
    {
        storerand=storerand-0.02;
    } */
    storerand=storerand + distribution(generator);
    storerand=std::max(0.0,std::min(storerand,1.0));//clamp
    return storerand;
}

//TODO: this function is getting ridiculously long - needs to be tidied up.
//TODO: above note still true, adding extra parameters anyway
void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float threshold, STDP_data* stdp,randconns_info* const rcinfo,const on_off Inhibitory)
{
    if (cached==false) {imcache=ReadImage(S.ImagePath);cached=true;}
    const Compute_float timemodper = fmod(timemillis,S.timeperiod);
    const Compute_float itercount = timemillis/S.timeperiod;
    if (itercount < 1.0  && S.TestPathChoice) {return;} //do nothing in first period- in test path mode the first period fails so skip over it - is pretty short anyway as no testing
    if (S.TestPathChoice )
    {
        if (timemodper < 0.001 && fabs(timemillis - lastset) > 0.01 && Inhibitory==OFF)
        {
            lastset=timemillis;
            if (S.Oscillating_path==ON)
            {
                if (S.Oscillating_Stimulus_Side==ON)
                {
                    //we might need to swap the stimulus associatins - lets check
                    if (fmod(itercount,S.path_osc_freq*2)<1)
                    {
                        int temp = rcinfo->SpecialAInd;
                        rcinfo->SpecialAInd=rcinfo->SpecialBInd;
                        rcinfo->SpecialBInd=temp;

                    }
                    if (fmod(itercount,4)<2)  //we alternate going left/right - but there is a 4 period cycle due to test trials
                    {
                        path1=true;
                        path2=false;
                    }
                    else
                    {
                        path1=false;
                        path2=true;
                    }
                }
                else if (S.Gradual_stim_swap==ON)
                {
                    int oneind = rcinfo->SpecialAInd;
                    int twoind = rcinfo->SpecialBInd;
                    printf("firing - init - %i %i\n",oneind,twoind);
                    if (oneind>twoind) {int temp=oneind;oneind=twoind;twoind=temp;}
                    if (RandFloat() < nextrand()) //round(-cos(2.0*M_PI/S.Gradual_swap_period*itercount)/2.0 + 0.5) ) //assign probabilities based on a cos curve
                    {
                        printf("firing - Side1 inhib is %i \n",Inhibitory);
                        rcinfo->SpecialBInd=oneind;
                        rcinfo->SpecialAInd=twoind;
                    }
                    else
                    {
                        printf("firing Side2\n");
                        rcinfo->SpecialAInd=oneind;
                        rcinfo->SpecialBInd=twoind;
                    }

                    if (fmod(itercount,4)<2)
                    {
                        path1=true;
                        path2=false;
                    }
                    else
                    {
                        path2=true;
                        path1=false;
                    }
                }
                else
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
                }
                if (fmod(itercount,2)< 1) //set up a test trial
                {
                    printf("firing - test\n");
                    path1 = false;
                    path2 = false;
                    fire1 = false; //fire1/2 serve to test whether the detection regions were activated
                    fire2 = false;
                    int oneind = rcinfo->SpecialAInd;
                    int twoind = rcinfo->SpecialBInd;
                    if (oneind>twoind) {int temp=oneind;oneind=twoind;twoind=temp;}
                    rcinfo->SpecialAInd=oneind; //always test the same stimulus
                    rcinfo->SpecialBInd=twoind;
                    stdp->RecordSpikes = OFF;
                }
                //TODO: this is the only warning left - but I am hesitant to change it as it is in a crucial path and things seem to be working.  Potentially could be a problem with very large itercount - but should be OK for a range in which the integers can be represented exactly by Compute_float
                if (fmod(itercount,2)==1) //print a test trial - this detects that the test trial has ended
                {
                    printf ("Res -  %i %i %i %f\n",(int)itercount,fire1,fire2,storerand);//cos(2.0*M_PI/S.Gradual_swap_period*itercount)/2.0 + 0.5);
                    stdp->RecordSpikes = ON;
                }
            }
            else
            {
                if (S.LotsofTesting==OFF)
                {
                    path1 = (RandFloat() < S.Prob1) && itercount < 21;
                    counts1 += path1==true?1:0;
                    if ((int)itercount==21) {fire1=false;fire2=false;} //this one seems to bail early
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
                        stdp->RecordSpikes=OFF;
                        //but we still want to activate the stimulus somehow - where do I do that? - later on - if both false, path2 rand stim will be activated
                    }
                    else
                    {
                        //so we ran a test on the previous trial - get result
                        printf("RESULT - %i %i %i\n",(int)itercount,fire1,fire2);
                        //now pick where we simulate
                        stdp->RecordSpikes=ON;
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
            CreateStims(S,itercount);
        }
    }
    if (S.Oscillating_path==ON) {if (path1==false && path2==false) {stdp->RecordSpikes=OFF;} else {stdp->RecordSpikes=ON;}}
    const bool stim1 = ((fabs(timemodper-80.0)<.01 && stim1choice) && itercount >= S.PreconditioningTrials)  ;  //late wave
    const bool stim2 =  fabs(timemodper-80.0 + S.lag)<.01  || fabs (timemodper-220 - 5 )<.01; //early wave - issues twice - first is normal, second is test trial.

    if (S.Testing == ON)
    {
        if (fabs(timemodper - 220) < 5) {StartTesting(voltsin,stdp);  }
        if (fabs(timemodper ) < 0.01) {EndTesting(stdp,(int)(itercount - S.PreconditioningTrials));  }
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
                    if (rcinfo != NULL && Inhibitory == OFF)
                    {
                        //if both set to false, this will activate stimulus 2 for testing
                        if (path1) {voltsin[rcinfo->SpecialAInd]=100; printf("firing path1 at %i\n",rcinfo->SpecialAInd); }
                        else       {voltsin[rcinfo->SpecialBInd]=100; printf("firing path2 at %i\n",rcinfo->SpecialBInd); }
                    }
                }
            }
            else if (pixel == cv::Vec3b(0,255,255)) //yellow - these 2 do nothing
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
            Compute_float grey = ((Compute_float)pixel.val[0])/255.0;
            voltsin[idx] += I0;
            voltsin[idx] += I1*cos(2*M_PI*S.mu*timemillis+Phimat[idx]);
            if (timemillis>=4000 && timemillis<=4500)
            {
                voltsin[idx] += I2*grey;
            }
        }
    }
}
