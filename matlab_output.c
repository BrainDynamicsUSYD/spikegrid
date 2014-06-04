///\file
#include <string.h>
#include "matlab_includes.h"
#include "output.h"
#ifdef MATLAB
//When using matlab, we want to be able to output just about any array of stuff.  This function does the work
mxArray* outputToMxArray (const tagged_array input) 
{
    const unsigned int size = input.size - (2*input.offset);
    mxArray* ret = mxCreateNumericMatrix((int)size,(int)size,MatlabDataType(),mxREAL); //matlab has signed ints for array sizes - really?
    Compute_float* dataptr =  (Compute_float*)mxGetData(ret);
    Compute_float* actualdata = taggedarrayTocomputearray(input);
    memcpy(dataptr,actualdata,sizeof(Compute_float)*size*size);
    free(actualdata);
    return ret;
}
mxArray* outputToMxStruct(const output_s input)
{
    const char** const fieldnames = (const char*[] ){"data","min","max"};
    mxArray* output = mxCreateStructMatrix(1,1,3,fieldnames);
    mxArray* minarr = mxCreateNumericMatrix(1,1,MatlabDataType(),mxREAL);
    Compute_float* minptr = (Compute_float*)mxGetData(minarr);
    minptr[0]=input.minval;
    mxArray* maxarr = mxCreateNumericMatrix(1,1,MatlabDataType(),mxREAL);
    Compute_float* maxptr = (Compute_float*)mxGetData(maxarr);
    maxptr[0]=input.maxval;
    mxSetField(output,0,"data",outputToMxArray(input.data));
    mxSetField(output,0,"min",minarr);
    mxSetField(output,0,"max",maxarr);
    return output;
}
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[])
{
    for (int i = 1;i<nrhs;i++)
    {
        if (mxGetClassID(prhs[i]) != mxCHAR_CLASS) {printf("rhs parameter %i needs to be a char string\n",i);return;}
    }
    if (nrhs>1) //output other stuff
    {
        int rhsidx = 1;
        while (rhsidx<nrhs)
        {
            char* data=malloc(sizeof(char)*1024); //should be big enough
            mxGetString(prhs[rhsidx],data,1023);
            plhs[rhsidx] = outputToMxStruct(getOutputByName(data));
            free(data);
            rhsidx++;
        }
    }
}
#endif
