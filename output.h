/// \file
#ifndef OUTPUT
#define OUTPUT
#include "layer.h"
char outdir [100];
output_s* Outputtable;
Compute_float* taggedarrayTocomputearray(const tagged_array input);
void makemovie(const layer l,const unsigned int t);
#endif //OUTPUT
