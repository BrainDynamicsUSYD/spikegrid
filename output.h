#ifndef OUTPUT
#define OUTPUT
#include "matlab_includes.h"
#include "pixeltypes.h"
#include "layer.h"
void makemovie(const layer_t l,const unsigned int t);
#ifdef MATLAB
mxArray* outputToMxArray (const tagged_array input); 
#endif //matlab
#endif //OUTPUT
