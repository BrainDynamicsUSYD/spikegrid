/// \file
#ifndef OUTPUT
#define OUTPUT
#include "matlab_includes.h"
#include "layer.h"
output_s* Outputtable;
void makemovie(const layer l,const unsigned int t);
#ifdef MATLAB
mxClassID __attribute__((pure,const)) MatlabDataType();
mxArray* outputToMxArray (const tagged_array input); 
#endif //matlab
#endif //OUTPUT
