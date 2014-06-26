///\file
#include <string.h>
#include "matlab_includes.h"
#include "output.h"
#ifdef MATLAB
//When using matlab, we want to be able to output just about any array of stuff.  This function does the work
mxArray* outputToMxArray (const output_s input) 
{
    switch (input.data_type)
    {
        case FLOAT_DATA:
            {
                const tagged_array data = input.data.TA_data;
                const unsigned int size = data.size - (2*data.offset);
                mxArray* ret = mxCreateNumericMatrix((int)size,(int)size,MatlabDataType(),mxREAL); //matlab has signed ints for array sizes - really?
                Compute_float* dataptr =  (Compute_float*)mxGetData(ret);
                Compute_float* actualdata = taggedarrayTocomputearray(data);
                memcpy(dataptr,actualdata,sizeof(Compute_float)*size*size);
                free(actualdata);
                return ret;
            }
        case RINGBUFFER_DATA:
            {
                const coords* const data = ringbuffer_getoffset(input.data.RB_data,0);
                //step 1 - get size
                int count = 0;
                while(data[count].x != -1) {count++;}
                mxArray* ret = mxCreateNumericMatrix(2,count,mxINT64_CLASS,mxREAL);//64bit ints because why not
                int64_t* dataptr = (int64_t*)mxGetData(ret);
                count=0;
                while(data[count].x != -1)
                {
                    dataptr[count*2]=(int64_t)data[count].x;
                    dataptr[count*2+1]=(int64_t)data[count].y;
                    count++;
                }
                return ret;
            }
        default:
            printf("don't know how to return that data\n");
            return NULL;
    }
}
mxArray* outputToMxStruct(const output_s input)
{
    const char** const fieldnames = (const char*[] ){"data","min","max"};
    mxArray* output = mxCreateStructMatrix(1,1,3,fieldnames);
    mxArray* minarr = mxCreateNumericMatrix(1,1,MatlabDataType(),mxREAL);
    Compute_float* minptr = (Compute_float*)mxGetData(minarr);
    mxArray* maxarr = mxCreateNumericMatrix(1,1,MatlabDataType(),mxREAL);
    Compute_float* maxptr = (Compute_float*)mxGetData(maxarr);
    if (input.data_type==FLOAT_DATA) //ringbuffer data doesn't really have a min/max
    {
        maxptr[0]=input.data.TA_data.maxval;
        minptr[0]=input.data.TA_data.minval;
    }
    else {minptr[0]=Zero;maxptr[0]=Zero;}
    mxSetField(output,0,"data",outputToMxArray(input));
    mxSetField(output,0,"min",minarr);
    mxSetField(output,0,"max",maxarr);
    return output;
}
void outputExtraThings(mxArray* plhs[],int nrhs,const mxArray* prhs[])
{
    //error checking
    if (nrhs ==1) {return;} //no more stuff to output
    if (nrhs > 3) {printf("Too many output things - error\n");return;} //incorrect no of rhs params
    if (mxGetClassID(prhs[1]) != mxCELL_CLASS) {printf("rhs parameter 1 is of wrong type - needs to be cell\n");return;}
    const mwSize numparams = (mwSize)mxGetNumberOfElements(prhs[1]);
    plhs[1] = mxCreateCellArray(1,(const mwSize[]){numparams});
    for (int i=0;i<numparams;i++)
    {
        char* data=malloc(sizeof(char)*1024); //should be big enough
        mxGetString(mxGetCell(prhs[1],i),data,1023);
        mxSetCell(plhs[1],i,outputToMxStruct(getOutputByName(data)));
    }
}
#endif
