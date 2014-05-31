#ifdef MATLAB
mxArray* outputToMxArray(const tagged_array input);
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[]);
#endif //matlab
