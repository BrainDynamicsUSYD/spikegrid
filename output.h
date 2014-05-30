/// \file
#ifndef OUTPUT
#define OUTPUT
#include "matlab_includes.h"
#include "layer.h"
char outdir [100];
output_s* Outputtable;
void makemovie(const layer l,const unsigned int t);
#ifdef MATLAB
mxArray* outputToMxArray(const tagged_array input);
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[])
#endif //matlab
#endif //OUTPUT
