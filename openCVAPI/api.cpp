/// \file
#include <map>
#include <iostream>
#include "opencv2/contrib/contrib.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "api.h"
extern "C"
{
#include "../typedefs.h"
}
using namespace cv;
Mat ProcessMatrix(const double* data,const double min,const double max,const unsigned int size)
{
	double* dispmat = (double*)malloc(sizeof(*dispmat)*size*size);
	for (unsigned int i=0;i<size;i++)
	{
		for (unsigned int j=0;j<size;j++)
		{
			dispmat[i*size+j]=(data[i*size+j] - min)/(max-min);
		}
	}
	Mat m(size,size,CV_64F,dispmat);
    Mat n;
    m.convertTo(n, CV_8UC1, 255.0 , 0);
	Mat outmat;
	applyColorMap(n,outmat,COLORMAP_JET);
    free(dispmat);
    return outmat;
}
int savecount;
std::map<std::string,Mat> matmap;
void mousecb(int event,int x,int y,int dummy, void* dummy2)
{
    if (event==CV_EVENT_LBUTTONDOWN)
    {
        char buf[100];
        sprintf(buf,"%i.png",savecount);
        savecount++;
        char* t = (char*)dummy2;
        std::string s(t);
        Mat m = matmap[s];
        imwrite(buf,m);
    }
}
void cvdispInit(const char** const names,const int count)
{
    for (int i=0;i<count;i++)
    {
        cvNamedWindow(names[i],CV_WINDOW_NORMAL);
        cvSetMouseCallback(names[i],mousecb,(void*)names[i]);
    }
}
void PlotColored(const char* winname,const double* data,const double min,const double max,const unsigned int size)
{
    Mat outmat = ProcessMatrix(data,min,max,size);
    imshow(winname,outmat);
    std::string s(winname); 
    Mat o ;
    outmat.copyTo(o);
    matmap[s]=o;
}

void SaveImage(const char* filename,const double* data,const double min,const double max,const unsigned int size)
{
    Mat outmat = ProcessMatrix(data,min,max,size);
    imwrite(filename,outmat);
}
void getcolors(const double* data, const double min, const double max, const unsigned int size, uchar* red,uchar* blue,uchar* green)
{
    Mat outmat = ProcessMatrix(data,min,max,size);
    for(unsigned int i=0;i<size;i++)
    {
        for(unsigned int j=0;j<size;j++)
        {
            Vec3b intensity = outmat.at<Vec3b>(i, j);
            blue[i*size+j]  = intensity.val[0];
            green[i*size+j] = intensity.val[1];
            red[i*size+j]   = intensity.val[2];
        }
    }
}
void PlotWithColors(const double* const  R,const double* const G,const unsigned  int size,const char* winname)
{
    int elemtype=CV_64FC3;
//    if (sizeof(Compute_float)==sizeof(float)) {elemtype=CV_32FC3;} //if compute_float is float use single precision in opencv
  //  else {elemtype=CV_64FC3;}//TODO - figure this out somewhere to make it accessible everywhere.  There is a similar problem in the matlab code.  This is a pretty bad hack.
    Mat* image = new Mat(size,size,elemtype);
    for(unsigned int i=0;i<size;i++)
    {
        for(unsigned int j=0;j<size;j++)
        {
            image->at<Vec3d>(i, j).val[0]=R[i*size+j];
            image->at<Vec3d>(i, j).val[1]=G[i*size+j];
        }
    }
    imshow(winname,*image);

}
