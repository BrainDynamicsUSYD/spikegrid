#include "output.h"
#include "matlab_includes.h"
#include "output.h"
#ifdef MATLAB
mxArray* outputToMxArray(const output_s input);
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[]);
#endif //matlab
