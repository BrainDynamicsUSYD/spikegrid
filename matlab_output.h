#ifdef MATLAB
#include "matlab_includes.h"
#include "output.h"
mxArray* outputToMxArray(const output_s input);
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[]);
#endif //matlab
