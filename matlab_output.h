/// \file
#ifdef MATLAB
#include "matlab_includes.h"
typedef struct output_s output_s;
mxArray* outputToMxArray(const output_s input);
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[]);
#endif //matlab
