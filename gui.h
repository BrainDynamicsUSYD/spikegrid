/// \file
#ifdef OPENCV
typedef struct randconns_info randconns_info;
void cvdispInit(const char** const names,const int count);
void cvdisp (const char** const names, const int count,const randconns_info* const rcinfo);
#endif
