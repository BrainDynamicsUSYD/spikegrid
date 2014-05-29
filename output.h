/// \file
#ifndef OUTPUT
#define OUTPUT
#include "matlab_includes.h"
#include "layer.h"
output_s* Outputtable;
void makemovie(const layer l,const unsigned int t,const int jobnumber);
#ifdef MATLAB
mxClassID __attribute__((pure,const)) MatlabDataType();
mxArray* outputToMxStruct(const output_s input);
mxArray* outputToMxArray(const tagged_array input);
#endif //matlab
#endif //OUTPUT
