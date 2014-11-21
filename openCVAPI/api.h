/// \file
#ifdef __cplusplus
#include "opencv2/core/core.hpp"
cv::Mat ProcessMatrix(const double* data,const double min,const double max,const unsigned int size);
	extern "C" {
#endif
void PlotColored(const char* winname,const double* data,const double min,const double max,const unsigned int size);
void SaveImage(const char* filename,const double* data,const double min,const double max,const unsigned int size);
void getcolors(const double* data, const double min, const double max, const unsigned int size, unsigned char* red,unsigned char* blue,unsigned char* green);
#ifdef __cplusplus
	}
#endif
