/// \file
#include "output.h"
#include "tagged_array.h"
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "openCVAPI/api.h"
#ifdef OPENCV
void cvdispInit(const char** const names,const int count)
{
    for (int i=0;i<count;i++)
    {
        output_s out = getOutputByName(names[i]);
        cvNamedWindow(out.name,CV_WINDOW_NORMAL);
    }
}
void cvdisp (const char** const names, const int count)
{
    for (int i=0;i<count;i++)
    {
        output_s out = getOutputByName(names[i]);
        const unsigned int size = tagged_array_size_(*out.data.TA_data)*out.data.TA_data->subgrid;
        if (out.data.TA_data->subgrid>1)
        {
            const unsigned int smallsize = tagged_array_size_(*out.data.TA_data);
            fcoords* d = taggedArrayCOM(*out.data.TA_data);
            Compute_float* R = malloc(sizeof(Compute_float)*smallsize*smallsize);
            Compute_float* G = malloc(sizeof(Compute_float)*smallsize*smallsize);
            for (unsigned int a=0;a<smallsize;a++)
            {
                for (unsigned int b=0;b<smallsize;b++)
                {
                    R[a*smallsize+b]=d[a*smallsize+b].x/100 +0.5;
                    G[a*smallsize+b]=d[a*smallsize+b].y/100 +0.5;
                }
            }
            PlotWithColors(R,G,smallsize,out.name);
        }
        else
        {
            Compute_float* data = taggedarrayTocomputearray(*out.data.TA_data);
            PlotColored(out.name,data,out.data.TA_data->minval,out.data.TA_data->maxval,size);
        free(data);
        }
        cvWaitKey(1);
    }
}
#endif
