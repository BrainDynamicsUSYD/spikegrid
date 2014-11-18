/// \file
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <vector>
#include <unistd.h>
#include "stdio.h"
#include "../openCVAPI/api.h" //this is c++
extern "C"
{
#include "../tagged_array.h"
#include "../output.h" //but this is C 
#include "../cppparamheader.h"
#include "../sizes.h"
#include "../lagstorage.h"
}
#include "out.h" //and this is c++ again.
std::vector<Output*> outvec;

PNGoutput::PNGoutput(int idxin ,const int intervalin,const tagged_array* datain) : Output(intervalin,idxin)
{
   data=datain;
}
void PNGoutput::DoOutput()
{

#ifdef OPENCV
    count++;
    char fnamebuffer[30];
    const unsigned int size = tagged_array_size(*data)*data->subgrid;
    Compute_float* actualdata=taggedarrayTocomputearray(*data);
    sprintf(fnamebuffer,"%s/%i-%i.png",outdir,this->GetIdx(),count);
    SaveImage(fnamebuffer,actualdata,data->minval,data->maxval,size);
    free(actualdata);
#else
    printf("Using PNG outout without opencv is not possible\n");
#endif
}

TextOutput::TextOutput(int idxin ,const int intervalin,const tagged_array* datain) : Output(intervalin,idxin)
{
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
    const unsigned int size = tagged_array_size(*data)*data->subgrid;
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
SpikeOutput::SpikeOutput(int idxin,const int intervalin, const lagstorage* datain) : TextOutput(idxin,intervalin,NULL)
{
    data=datain;
}
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
                out = new PNGoutput(i,m[i].Delay,Outputtable[m[i].Output].data.TA_data);
                outvec.push_back(out);
                break;
            case TEXT:
                out = new TextOutput(i,m[i].Delay,Outputtable[m[i].Output].data.TA_data);
                outvec.push_back(out);
                break;
            case CONSOLE:
                out = new ConsoleOutput(i,m[i].Delay,Outputtable[m[i].Output].data.TA_data);
                break;
            case SPIKES:
                out = new SpikeOutput(i,m[i].Delay,Outputtable[m[i].Output].data.Lag_data);
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
