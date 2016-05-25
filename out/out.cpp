/// \file
#define _CRT_SECURE_NO_WARNINGS 1
#include <stdio.h> //printf in c++ yay
#include <errno.h>
#include <map>
#include <iostream>
#ifndef _WIN32
#include <unistd.h> //what is this actually for?
#include "opencv2/highgui/highgui.hpp" //for video writer
#else
#include <opencv2\highgui\highgui.hpp>
#endif
#include "api.h" //this is c++
#include "outputtable.h"
extern "C"
{
#include "../tagged_array.h"
#include "../cppparamheader.h"
#include "../lagstorage.h"
}
#include "out.h" //and this is c++ again.

on_off showimages = ON;
std::vector<Output*> outvec;

void Output::update() 
{

}
void Output::DoOutput()
{
    this->update();
    this->DoOutput_();
}

void TAOutput::update()
{
    if (this->out->Updateable==ON)
    {
       this->data=this->out->UpdateFn(this->out->function_arg);
    }
}
TAOutput::TAOutput(int idxin,const int intervalin,const output_s* datain) : Output(intervalin,idxin)
{
    data=datain->data.TA_data;
    out=datain;
}

PNGoutput::PNGoutput(int idxin ,const int intervalin,const output_s* datain,const char* const overlayin) : TAOutput(idxin,intervalin,datain)
{
   overlay = GetOverlayByName(overlayin);
}
void PNGoutput::DoOutput_()
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
int savecount;
std::map<std::string,cv::Mat*> matmap;
void mousecb(int event,int ,int ,int , void* dummy2)
{
    if (event==CV_EVENT_LBUTTONDOWN)
    {
        char buf[100];
        sprintf(buf,"%i.png",savecount);
        savecount++;
        char* t = (char*)dummy2;
        std::string s(t);
        cv::Mat m = *matmap[s];
        cv::imwrite(buf,m);
    }
}
GUIoutput::GUIoutput(int a,int b, const output_s* c, const char* const d, const char* const wname) : PNGoutput(a,b,c,d)
{
    if (showimages==ON)
    {
        winname=(char*)malloc(20);
        strcpy(winname,wname);
        cvNamedWindow(wname,CV_WINDOW_NORMAL);
        char* data=(char*)malloc(20);//create our own 20 byte chunk of ram to avoid a warning - TODO: free this ram
        strcpy(data,wname);
        cvSetMouseCallback(wname,mousecb,(void*)data);
    }
}
void GUIoutput::DoOutput_()
{
    if (showimages == ON)
    {
        cv::Mat* m = new cv::Mat(TA_toMat(data,overlay));
        imshow(winname,*m);
        std::string s(winname);
        delete matmap[s]; //delete the old value
        matmap[s]=m;
        cv::waitKey(10); //TODO: move this somewhere else - bad for perf
    }
}

VidOutput::VidOutput(int idxin ,const int intervalin,const output_s* datain,const char* const overlayin) : TAOutput(idxin,intervalin,datain)
{
    overlay = GetOverlayByName(overlayin);
    char buf[100];
    sprintf(buf,"%s/%i.avi",outdir,idxin);
    int fourcc = CV_FOURCC('H','F','Y','U');
    writer = new cv::VideoWriter();
            //   fname,code,fps,size,color
    writer->open(buf,fourcc,60,cvSize(grid_size,grid_size),1);
    if (writer->isOpened()==false)
    {
        std::cout <<"failed to open output stream for a video - quitting" << std::endl;
        exit(EXIT_FAILURE);
    }
}
void VidOutput::DoOutput_()
{
#ifdef OPENCV
    writer->write(TA_toMat(data,overlay)); //valgrind gives an error here - only on the first frame.  I don't know what causes it but can't see an obvious problem.
#else
    printf("Using video outout without opencv is not possible\n");
#endif
}
SingleFileOutput::SingleFileOutput(int idxin ,const int intervalin) : Output(intervalin,idxin)
{
   char buf[100];
   sprintf(buf,"%s/%i.txt",outdir,idxin);
   f=fopen(buf,"w");
   if (f==NULL) {printf("fopen failed for %s error is %s\n",buf,strerror(errno));}
}

TextOutput::TextOutput(int idxin,const int intervalin,const output_s* datain) : SingleFileOutput(idxin,intervalin) {out=datain;data=out->data.TA_data;}
void TextOutput::DoOutput_()
{
    if (this->out->Updateable==ON)
    {
       this->data=this->out->UpdateFn(this->out->function_arg);
    }
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
ConsoleOutput::ConsoleOutput(int idxin, const int intervalin, const output_s* datain) : TAOutput(idxin, intervalin, datain)
{
    setvbuf(stdout,NULL,_IONBF,0);
}
void ConsoleOutput::DoOutput_()
{
    #if defined(OPENCV) && !defined(_WIN32)
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
    printf("Using console output requires opencv and linux (to get the color mappings)");
#endif
}

SpikeOutput::SpikeOutput(int idxin,const int intervalin, const lagstorage* datain) : SingleFileOutput(idxin,intervalin) {data=datain;}
void SpikeOutput::DoOutput_()
{
    for (Neuron_coord i=0;i<grid_size;i++)
    {
        for (Neuron_coord j=0;j<grid_size;j++)
        {
            if (CurrentShortestLag(data,grid_index((coords){.x=i,.y=j})*data->lagsperpoint,grid_index((coords){.x=i,.y=j})) == 1)


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
        output_s* outt = GetOutputByName(m[i].Output);
        switch (m[i].method)
        {
            case PICTURE:
                out = new PNGoutput(i,m[i].Delay,outt,m[i].Overlay);
                outvec.push_back(out);
                break;
            case TEXT:
                out = new TextOutput(i,m[i].Delay,outt);
                outvec.push_back(out);
                break;
            case CONSOLE:
                out = new ConsoleOutput(i,m[i].Delay,outt);
                outvec.push_back(out);
                break;
            case SPIKES:
                out = new SpikeOutput(i,m[i].Delay,outt->data.Lag_data);
                outvec.push_back(out);
                break;
            case VIDEO:
                out = new VidOutput(i,m[i].Delay,outt,m[i].Overlay);
                outvec.push_back(out);
                break;
            case GUI:
                out = new GUIoutput(i,m[i].Delay,outt ,m[i].Overlay,outt->name);
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

