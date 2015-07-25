/// \file
#include <stdlib.h>
//sometimes this is in a different spot
//#include "opencv2/imgproc.hpp"
#include "opencv2/contrib/contrib.hpp"
#include "api.h"
extern "C"
{
	#include "../tagged_array.h"
	#include "../output.h"
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
    //unfortunately the constructor here takes an int - but should really take unsigned int
	Mat m((int)size,(int)size,CV_64F,dispmat);
    Mat n;
    m.convertTo(n, CV_8UC1, 255.0 , 0);
	Mat outmat;
	applyColorMap(n,outmat,COLORMAP_JET);
    free(dispmat);
    return outmat;
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
cv::Mat TA_toMat(const tagged_array* const data, const overlaytext* const o)
{
	const unsigned int size = tagged_array_size_(*data)*data->subgrid;
	Compute_float* actualdata = taggedarrayTocomputearray(*data);
	cv::Mat m = ProcessMatrix(actualdata, data->minval, data->maxval, size);
	free(actualdata);
	if (o != NULL)
	{
		std::string s = std::to_string(o->func());
		putText(m, s, cv::Point(0, size), cv::FONT_HERSHEY_PLAIN, 1.0, cv::Scalar(255, 255, 255));
	}
	return m;
}
