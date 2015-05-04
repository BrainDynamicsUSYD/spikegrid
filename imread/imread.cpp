#include "opencv2/contrib/contrib.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdbool>
#include <stdlib.h>
#include "imread.h"
extern "C"
{
#include "../mymath.h"
#include "../STDP.h"
#include "../sizes.h"
}
typedef enum {Normal=0,Testing=1} Net_state;
bool cached=false;
Net_state state = Normal;
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
int  counts1;
Compute_float lastset;
bool stim1choice=true;
void CreateStims(const Compute_float timemodper,const Stimulus_parameters S,const Compute_float itercount)
{
    printf("createstims\n");
    stim1choice = RandFloat() > S.NoUSprob;
    if (stim1choice && (int)itercount > lastonesfire) {onesfire++;lastonesfire=(int)itercount; printf("increase\n");}
}
void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float threshold, STDP_data* stdp)
{
    if (cached==false) {imcache=ReadImage(S.ImagePath);cached=true;}
    const Compute_float timemodper = fmod(timemillis,S.timeperiod);
    const Compute_float itercount = timemillis/S.timeperiod;
    if (itercount < 1.0  && S.TestPathChoice) {return;} //do nothing in first period- in test path mode the first period fails so skip over it - is pretty short anyway as no testing

    bool path2=false; //make compiler happy - need to redo this whole function anyway
    if (S.TestPathChoice)
    {
        if (timemodper < 0.001 && fabs(timemillis - lastset) > 0.01)
        {
            lastset=timemillis;
            printf("picking path\n");
            path1 = (RandFloat() < S.Prob1) && itercount < 21;
            counts1 += path1==true?1:0;
            if ((int)itercount==21) {fire1=false;fire2=false;}
            if ((int)itercount==22) {printf("%i %i %i\n",counts1,fire1,fire2);exit(EXIT_SUCCESS);}
        }
        path2 = !path1 && itercount < 21;
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
    const bool stim1 = ((fabs(timemodper-80.0)<.01 && stim1choice) && itercount >= S.PreconditioningTrials)  ;  //late wave
    const bool stim2 =  fabs(timemodper-80.0 + S.lag)<.01  || fabs (timemodper-220 - 5 )<.01; //early wave - issues twice - first is normal, second is test trial.

    if (S.Testing == ON)
    {
        if (fabs(timemodper - 220) < 5) {StartTesting(voltsin,stdp);  }
        if (fabs(timemodper ) < 0.01) {EndTesting(stdp,(int)(itercount - S.PreconditioningTrials),S);  }
    }
    if (timemodper < 5) { ResetVoltages(voltsin);} //reset before next period.
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            cv::Vec3b pixel = imcache.at<cv::Vec3b>(x,y);
            //uncomment to print colours - not particularly helpful
            //std::cout << x << "," << y << " " <<  pixel << std::endl;
            if (pixel == cv::Vec3b(0,0,0))
            {
                voltsin[x*grid_size+y]=-100;
            }
            else if ( stim1 && pixel == cv::Vec3b(0,0,255))
            {
                voltsin[x*grid_size+y]=100;
            }
            else if (stim2 && pixel == cv::Vec3b(255,0,0))
            {
                voltsin[x*grid_size+y]=100;
            }
            else if (path1 && pixel == cv::Vec3b(0,255,0))
            {
                voltsin[x*grid_size+y] = -100;
            }
            else if (path2 && pixel == cv::Vec3b(0,100,0))
            {
                voltsin[x*grid_size+y] = -100;
            }
            //detection loop - keep in a separate statement for now
            if (pixel == cv::Vec3b(100,100,100))
            {
                if (voltsin[x*grid_size+y] > threshold )
                {
                    fire1 = true;
                }
            }
            else if (pixel == cv::Vec3b(150,150,150))
            {
                if (voltsin[x*grid_size+y] > threshold )
                {
                    fire2 = true;
                }
            }
        }
    }
}
