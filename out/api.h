/// \file
#ifdef __cplusplus
#include "opencv2/core/core.hpp"
typedef struct tagged_array tagged_array;
typedef struct overlaytext  overlaytext;
cv::Mat ProcessMatrix(const double* data,const double min,const double max,const unsigned int size);
cv::Mat TA_toMat(const tagged_array* const data, const overlaytext* const o);
	extern "C" {
#endif
void PlotColored(const char* winname,const double* data,const double min,const double max,const unsigned int size);
void getcolors(const double* data, const double min, const double max, const unsigned int size, unsigned char* red,unsigned char* blue,unsigned char* green);
void PlotWithColors(const double* const  R,const double* const G,const unsigned  int size,const char* winname);
#ifdef __cplusplus
	}
#endif
