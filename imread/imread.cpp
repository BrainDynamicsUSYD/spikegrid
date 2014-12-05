#include "opencv2/contrib/contrib.hpp"
#include "opencv2/core/core.hpp"
#include "opencv2/highgui/highgui.hpp"
#include <stdio.h>
#include <iostream>
#include <string>
#include <cstdbool>
#include <stdint.h>
#include "imread.h"
extern "C"
{
#include "../sizes.h"
}

cv::Mat ReadImage()
{
    std::string path= "input_maps/test.png";
    cv::Mat m =cv::imread(path); //the pixels are stored with type cv::Vec3b - third element is red.  0 is black, 255 is white?
    return m;
}
cv::Mat imcache; //elements have type Vec3b
void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S)
{
    cv::Mat m = ReadImage();
    const Compute_float timemodper = fmod(timemillis,S.timeperiod);
    const bool stim1 = abs(timemodper-80.0)<.01;
    const bool stim2 =  abs(timemodper-80.0 + S.lag)<.01;
    if (stim1) {std::cout<< "stim1" << std::endl;}
    if (stim2) {std::cout<< "stim2" << std::endl;}
   // std::cout << timemillis << std::endl;
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            cv::Vec3b pixel = m.at<cv::Vec3b>(x,y);
     //       std::cout << pixel << std::endl;
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
        }
    }
}
