#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include "stdio.h"
#include "../openCVAPI/api.h"
extern "C"
{
#include "../output.h"
}
#include "out.h"
class Output
{
    int interval;
    public:
        Output(int intervalin) {interval=intervalin;};
        virtual void DoOutput() {};
        int GetInterval() const {return interval;};
};

std::vector<Output*> outvec;
class PNGoutput : public Output
{
    int idx,count;
    const tagged_array* data;
    public:
        PNGoutput(int,int,const tagged_array* );
        void DoOutput() ;
};

PNGoutput::PNGoutput(int idxin ,const int intervalin,const tagged_array* datain) : Output(intervalin)
{
   idx=idxin;
   data=datain;
   count=0;
}
void PNGoutput::DoOutput()
{

#ifdef OPENCV
    count++;
    char fnamebuffer[30];
    const unsigned int size = tagged_array_size(*data)*data->subgrid;
    Compute_float* actualdata=taggedarrayTocomputearray(*data);
    sprintf(fnamebuffer,"%s/%i-%i.png",outdir,idx,count);
    SaveImage(fnamebuffer,actualdata,data->minval,data->maxval,size);
    free(actualdata);
#else
    printf("Using PNG outout without opencv is not possible\n");
#endif
}
class TextOutput : public Output
{
    FILE* f;
    int idx,count;
    const tagged_array* data;
    public:
        TextOutput(int,int,const tagged_array* );
        virtual void DoOutput() ;
};
TextOutput::TextOutput(int idxin ,const int intervalin,const tagged_array* datain) : Output(intervalin)
{
   idx=idxin;
   data=datain;
   char buf[100];
   sprintf(buf,"%s/%i.txt",outdir,idxin);
   f=fopen(buf,"w");
   if (f==NULL) {printf("fopen failed for %s error is %s\n",buf,strerror(errno));}
}
void TextOutput::DoOutput()
{
    Compute_float* actualdata = taggedarrayTocomputearray(*data);
    const unsigned int size = tagged_array_size(*data);
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
void MakeOutputs(const output_parameters* const m)
{
    int i = 0;
    while (m[i].method != NO_OUTPUT)
    {
        Output* out;
        switch (m[i].method)
        {
            case PICTURE:
                out = new PNGoutput(i,m[i].Delay,&Outputtable[m[i].Output].data.TA_data);
                outvec.push_back(out);
                break;
            case TEXT:
                out = new TextOutput(i,m[i].Delay,&Outputtable[m[i].Output].data.TA_data);
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
    for (int i=0;i<outvec.size();i++)
    {
        if  ((time % outvec[i]->GetInterval()) == 0)
        {outvec[i]->DoOutput();}
    }
}
void CleanupOutputs()
{
    for (int i=0;i<outvec.size();i++)
    {
        delete outvec[i];
    }
    outvec.clear();
}
