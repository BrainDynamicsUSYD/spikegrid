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
    S->RecordSpikes = OFF;
}
void EndTesting(STDP_data* S)
{
    state = Normal;
    if (fire1 && fire2)
    {
        std::cout << "Both spiked" << std::endl;
        exit(0);
    }
    S->RecordSpikes = ON;
}

void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float threshold, STDP_data* stdp)
{
    if (cached==false) {imcache=ReadImage(S.ImagePath);cached=true;}
    const Compute_float timemodper = fmod(timemillis,S.timeperiod);
    const Compute_float itercount = timemillis/S.timeperiod;
    const bool stim1 = (fabs(timemodper-80.0)<.01 && itercount > S.PreconditioningTrials)  ;
    const bool stim2 =  fabs(timemodper-80.0 + S.lag)<.01  || fabs (timemodper-220 - 5 )<.01;
    if (fabs(timemodper - 220) < 5) {StartTesting(voltsin,stdp);  }
    if (fabs(timemodper ) < 0.01) {EndTesting(stdp);  }
    if (timemodper < 5) { ResetVoltages(voltsin);} //reset before next period.
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            cv::Vec3b pixel = imcache.at<cv::Vec3b>(x,y);
        //    std::cout << x << "," << y << " " <<  pixel << std::endl;
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
            //detection loop - keep in a separate statement for now
            if (pixel == cv::Vec3b(100,100,100))
            {
                if (voltsin[x*grid_size+y] > threshold )
                {
                    std::cout << "spike" << std::endl;
                    fire1 = true;
                }
            }
            else if (pixel == cv::Vec3b(150,150,150))
            {
                if (voltsin[x*grid_size+y] > threshold )
                {
                    std::cout << "spike2" << std::endl;
                    fire2 = true;
                }
            }
        }
    }
}
