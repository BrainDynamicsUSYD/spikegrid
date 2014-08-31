#ifdef __cplusplus
	extern "C" {
#endif
void PlotColored(const char* winname,const double* data,const double min,const double max,const unsigned int size);
void SaveImage(const char* filename,const double* data,const double min,const double max,const unsigned int size);
void getcolors(const double* data, const double min, const double max, const unsigned int size, uchar* red,uchar* blue,uchar* green);
#ifdef __cplusplus
	}
#endif
