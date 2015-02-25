/// \file
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <iostream>
#include <unistd.h>
#include "stdio.h"
#include "../openCVAPI/api.h" //this is c++
extern "C"
{
#include "../tagged_array.h"
#include "../output.h" //but this is C
#include "../cppparamheader.h"
#include "../lagstorage.h"
}
#include "out.h" //and this is c++ again.
std::vector<Output*> outvec;

PNGoutput::PNGoutput(int idxin ,const int intervalin,const tagged_array* datain,const char* const overlayin) : Output(intervalin,idxin)
{
   data=datain;
    overlay = getOverlayByName(overlayin);
}
cv::Mat TA_toMat(const tagged_array* const data,const overlaytext* const o)
{
    const unsigned int size = tagged_array_size_(*data)*data->subgrid;
    Compute_float* actualdata=taggedarrayTocomputearray(*data);
    cv::Mat m =ProcessMatrix(actualdata,data->minval,data->maxval,size);
    free(actualdata);
    if (o != NULL)
    {
        std::string s = std::to_string(o->func());
        putText(m,s,cv::Point(0,size), cv::FONT_HERSHEY_PLAIN,1.0,cv::Scalar(255,255,255));
    }
    return m;
}
void PNGoutput::DoOutput()
{
#ifdef OPENCV
    count++;
    char fnamebuffer[30];
    sprintf(fnamebuffer,"%s/%i-%i.png",outdir,this->GetIdx(),count);
    imwrite(fnamebuffer,TA_toMat(data,overlay));
#else
    printf("Using PNG outout without opencv is not possible\n");
#endif
}
VidOutput::VidOutput(int idxin ,const int intervalin,const tagged_array* datain,const char* const overlayin) : Output(intervalin,idxin)
{
    overlay = getOverlayByName(overlayin);
    char buf[100];
    sprintf(buf,"%s/%i.avi",outdir,idxin);
    int fourcc = CV_FOURCC('H','F','Y','U');
    writer = new cv::VideoWriter(buf,fourcc,60,cvSize(grid_size,grid_size),true);
    data=datain;
}
void VidOutput::DoOutput()
{
#ifdef OPENCV
    writer->write(TA_toMat(data,overlay));
#else
    printf("Using PNG outout without opencv is not possible\n");
#endif
}
SingleFileOutput::SingleFileOutput(int idxin ,const int intervalin) : Output(intervalin,idxin)
{
   char buf[100];
   sprintf(buf,"%s/%i.txt",outdir,idxin);
   f=fopen(buf,"w");
   if (f==NULL) {printf("fopen failed for %s error is %s\n",buf,strerror(errno));}
}
TextOutput::TextOutput(int idxin,const int intervalin,const tagged_array* datain) : SingleFileOutput(idxin,intervalin) {data=datain;}
void TextOutput::DoOutput()
{
    Compute_float* actualdata = taggedarrayTocomputearray(*data);
    const unsigned int size = tagged_array_size_(*data);
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            fprintf(f,"%f,",actualdata[i*size+j]);
        }
        fprintf(f,"\n");
    }
    fflush(f);//prevents stalling in matlab
    free(actualdata);
}
ConsoleOutput::ConsoleOutput(int idxin,const int intervalin,const tagged_array* datain) : Output(intervalin,idxin)
{
    data=datain;
}
void ConsoleOutput::DoOutput()
{
    #ifdef OPENCV
    if (!isatty(fileno(stdout))) {return;} //if we are not outputting to a terminal - dont show pictures on console - need to add matlab detection
    char* buf = (char*)malloc(sizeof(char)*1000*1000);//should be plenty
    char* upto = buf;
    const unsigned int size = tagged_array_size_(*data)*data->subgrid;
    Compute_float* actualdata=taggedarrayTocomputearray(*data);
    unsigned char* red   = (unsigned char*)malloc(sizeof(unsigned char)*size*size);
    unsigned char* green = (unsigned char*)malloc(sizeof(unsigned char)*size*size);
    unsigned char* blue  = (unsigned char*)malloc(sizeof(unsigned char)*size*size);
    getcolors(actualdata,data->minval,data->maxval,size,red,blue,green);
    printf("\x1b[2J");
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            int r = sprintf(upto,"\x1b[48;2;%i;%i;%im ",red[i*size+j],green[i*size+j],blue[i*size+j]);
            upto += r;
        }
        int q = sprintf(upto,"\x1b[0m\n");
        upto += q;
    }
    puts(buf); //output giant buffer in one go - should be faster
    usleep(50000);//let terminal catch up - nasty hacky solution
    free(buf);free(red);free(green);free(blue);
#else
    printf("Using console output requires opencv (to get the color mappings)");
#endif
}
SpikeOutput::SpikeOutput(int idxin,const int intervalin, const lagstorage* datain) : SingleFileOutput(idxin,intervalin) {data=datain;}
void SpikeOutput::DoOutput()
{
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            if (CurrentShortestLag(data,(i*grid_size+j)*data->lagsperpoint) == 1)
            {
                fprintf(f,"%i,%i:",i,j);
            }
        }
    }
    fprintf(f,"\n");
    fflush(f);

}

void MakeOutputs(const output_parameters* const m)
{
    int i = 0;
    while (m[i].method != NO_OUTPUT)
    {
        Output* out;
        switch (m[i].method)
        {
            case PICTURE:
                out = new PNGoutput(i,m[i].Delay,Outputtable[m[i].Output].data.TA_data,m[i].Overlay);
                outvec.push_back(out);
                break;
            case TEXT:
                out = new TextOutput(i,m[i].Delay,Outputtable[m[i].Output].data.TA_data);
                outvec.push_back(out);
                break;
            case CONSOLE:
                out = new ConsoleOutput(i,m[i].Delay,Outputtable[m[i].Output].data.TA_data);
                outvec.push_back(out);
                break;
            case SPIKES:
                out = new SpikeOutput(i,m[i].Delay,Outputtable[m[i].Output].data.Lag_data);
                outvec.push_back(out);
                break;
            case VIDEO:
                out = new VidOutput(i,m[i].Delay,Outputtable[m[i].Output].data.TA_data,m[i].Overlay);
                outvec.push_back(out);
                break;
            default:
                break;
        }
        i++;
    }
}
void DoOutputs(const unsigned int time)
{
    for (size_t i=0;i<outvec.size();i++)
    {
        if  ((time % outvec[i]->GetInterval()) == 0)
        {outvec[i]->DoOutput();}
    }
}
void CleanupOutputs()
{
    for (size_t i=0;i<outvec.size();i++)
    {
        delete outvec[i]; //this is hideous code that produces a warning.  But who cares.
    }
    outvec.clear(); //this is the more important bit - the memory occupied by the various output objects is comparitively tiny.
}
//###################
//-------------------------------------MATLAB stuff below here
//###################
//So here is a problem - All the outputs are set up to essentially return `void`.
//However, we really need to return something for matlab.
//Also, it is possible that the matlab outputs change over time
//As a result, there is currently not a class for the matlab outputs - this could change in the future
#ifdef MATLAB
#include "../matlab_includes.h"
//When using matlab, we want to be able to output just about any array of stuff.  This function does the work
mxArray* outputToMxArray (const output_s input)
{
    switch (input.datatype)
    {
        case FLOAT_DATA:
            {
                const tagged_array* const data = input.data.TA_data;
                const unsigned int size = tagged_array_size_(*data)*data->subgrid;
                mxArray* ret = mxCreateNumericMatrix((int)size,(int)size,MatlabDataType(),mxREAL); //matlab has signed ints for array sizes - really?
                Compute_float* dataptr =  (Compute_float*)mxGetData(ret);
                Compute_float* actualdata = taggedarrayTocomputearray(*data);
                memcpy(dataptr,actualdata,sizeof(Compute_float)*size*size);
                free(actualdata);
                return ret;
            }
        default:
            printf("don't know how to return that data\n");
            return NULL;
    }
}
mxArray* outputToMxStruct(const output_s input)
{
    const char* fieldnames[] = {"data","min","max"};
    mxArray* output = mxCreateStructMatrix(1,1,3,fieldnames);
    mxArray* minarr = mxCreateNumericMatrix(1,1,MatlabDataType(),mxREAL);
    Compute_float* minptr = (Compute_float*)mxGetData(minarr);
    mxArray* maxarr = mxCreateNumericMatrix(1,1,MatlabDataType(),mxREAL);
    Compute_float* maxptr = (Compute_float*)mxGetData(maxarr);
    if (input.datatype==FLOAT_DATA) //ringbuffer data doesn't really have a min/max
    {
        maxptr[0]=input.data.TA_data->maxval;
        minptr[0]=input.data.TA_data->minval;
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
    if (nrhs != 2) {printf("Need exactly two entries on the RHS of the MATLAB call - not outputting anything");}
    if (mxGetClassID(prhs[1]) != mxCELL_CLASS) {printf("rhs parameter 1 is of wrong type - needs to be cell\n");return;}
    const mwSize numparams = (mwSize)mxGetNumberOfElements(prhs[1]);
    const mwSize nparams[] = {numparams};
    plhs[1] = mxCreateCellArray(1,nparams);
    for (int i=0;i<numparams;i++)
    {
        char* data=(char*)malloc(sizeof(char)*1024); //should be big enough
        mxGetString(mxGetCell(prhs[1],i),data,1023);
        mxSetCell(plhs[1],i,outputToMxStruct(getOutputByName(data)));
    }
}
#endif
