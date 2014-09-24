/// \file
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#ifdef OPENCV
    #include "opencv/cv.h"
    #include "opencv/highgui.h"
    #include "openCVAPI/api.h"
#endif
#include "STD.h"
#include "output.h"
#include "paramheader.h"
#include "STDP.h"
///Total number of things to be output - occasionally needs to be incremented
#define output_count  19
///Holds the outputtable objects for the current model
output_s* Outputtable;
///Holds file* for the output types that output to a consistent file over time to save repeatedly calling fopen/fclose - mainly useful for ouputting ringbuffer stuff
FILE* outfiles[output_count];
///Extracts the actual information out of a tagged array and converts it to a simple square matrix
Compute_float* taggedarrayTocomputearray(const tagged_array input)
{
    const unsigned int size = (input.size - (2*input.offset));
    Compute_float * ret = calloc(sizeof(*ret),size*size*input.subgrid*input.subgrid);
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            for (unsigned int k=0;k<input.subgrid;k++)
            {
                for (unsigned int l=0;l<input.subgrid;l++)
                {
                    //this part reshuffles the matrix so that it looks better when you do a plot in matlab.  The subgrid stuff is mainly used for STDP where there is a matrix associated with each point
                    const Compute_float val =  input.data[((i+input.offset)*input.size + j + input.offset)*input.subgrid*input.subgrid + k*input.subgrid + l ];
                    ret[(i*input.subgrid+k)*size*input.subgrid +j*input.subgrid+l]=val;//this at least appears to bee correct
                }
            }
        }
    }
    return ret;
}

/// number of PNG's outputted.  Used to keep track of the next filename to use
///convert a tagged array to a PNG.  Paths are auto-calculated
void outputToPng(const tagged_array input,const int idx,const unsigned int count)
{
    char fnamebuffer[30];
    const unsigned int size = (input.size - (2*input.offset))*input.subgrid;
    Compute_float* actualdata=taggedarrayTocomputearray(input);
    sprintf(fnamebuffer,"%s/%i-%i.png",outdir,idx,count);
#ifdef OPENCV
    SaveImage(fnamebuffer,actualdata,input.minval,input.maxval,size);
#endif
    free(actualdata);
}
///TODO: Need to get a better way of detecting when rendering has finished
void outputToConsole(const tagged_array input)
{
#ifdef OPENCV
    if (!isatty(fileno(stdout))) {return;} //if we are not outputting to a terminal - dont show pictures on console - need to add matlab detection
    char* buf = malloc(sizeof(char)*1000*1000);//should be plenty
    char* upto = buf;
    const unsigned int size = (input.size - (2*input.offset))*input.subgrid;
    Compute_float* actualdata=taggedarrayTocomputearray(input);
    uchar* red = malloc(sizeof(uchar)*size*size);
    uchar* green = malloc(sizeof(uchar)*size*size);
    uchar* blue = malloc(sizeof(uchar)*size*size);
    getcolors(actualdata,input.minval,input.maxval,size,red,blue,green);
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
#endif
}
///Send an outputtable to a text file
///@param input     the outputtable object to output
///@param idx       the index of the outputtable so that if multiple objects are output, files have consistent naming
void outputToText(const output_s input,const int idx)
{
    if (outfiles[idx]==NULL)
    {
        char buf[100];
        sprintf(buf,"%s/%i.txt",outdir,idx);
        outfiles[idx]=fopen(buf,"w");
        if (outfiles[idx]==NULL) {printf("fopen failed on %s error is %s\n",buf,strerror(errno));}
    }
    switch (input.data_type)
    {
        case FLOAT_DATA:
        {
            Compute_float* const data = taggedarrayTocomputearray(input.data.TA_data);
            const unsigned int size = input.data.TA_data.size - (2*input.data.TA_data.offset);
            for (unsigned int i=0;i<size;i++)
            {
                for (unsigned int j=0;j<size;j++)
                {
                    fprintf(outfiles[idx],"%f,",data[i*size+j]);
                }
                fprintf(outfiles[idx],"\n");
            }
            fflush(outfiles[idx]);//prevents stalling in matlab
            free(data);
            break;
        }
        case SPIKE_DATA:
        {
            for (int i=0;i<grid_size;i++)
            {
                for (int j=0;j<grid_size;j++)
                {
                    if (CurrentShortestLag(input.data.Lag_data,(i*grid_size+j)*input.data.Lag_data->lagsperpoint) == 1)
                    {
                        fprintf(outfiles[idx],"%i,%i:",i,j);
                    }
                }
            }
            fprintf(outfiles[idx],"\n");
            fflush(outfiles[idx]);
            break;
        }
        default:
        printf("I don't know how to output this data\n");

    }
}
///High level function to do output
void dooutput(const output_parameters* const m,const unsigned int t)
{
    int i = 0;
    while (m[i].output_method != NO_OUTPUT)
    {
        if (t % m[i].Delay==0)
        {
            //clang gives a warning about NO_output not being handled here - safe to ignore
            switch (m[i].output_method)
            {
                case PICTURE:
                    if (Outputtable[m[i].Output].data_type == FLOAT_DATA) { outputToPng(Outputtable[m[i].Output].data.TA_data,i,t/m[i].Delay);}
                    else {printf("can't output a ringbuffer to a picture\n");}
                    break;
                case TEXT:
                    outputToText(Outputtable[m[i].Output],i);
                    break;
                case CONSOLE:
                    if (Outputtable[m[i].Output].data_type == FLOAT_DATA) { outputToConsole(Outputtable[m[i].Output].data.TA_data);}
                    else {printf("can't output a ringbuffer to the console\n");}
                    break;
                default:
                    printf("unknown output method\n");
            }
        }
        i++;
    }
}
///Finds an output which matches the given name - case sensitive
///@param name the name of the outputtable
output_s __attribute__((pure)) getOutputByName(const char* const name)
{
    int outidx=0;
    while (strlen(Outputtable[outidx].name) != 0)
    {
        if (!strcmp(Outputtable[outidx].name,name))
        {
            return Outputtable[outidx];
        }
        outidx++;
    }
    printf("tried to get unknown thing to output called -%s-\n",name);
    exit(EXIT_FAILURE);
}
#ifdef OPENCV
void cvdispInit(const char** const names,const int count)
{
    for (int i=0;i<count;i++)
    {
        output_s out = getOutputByName(names[i]);
        cvNamedWindow(out.name,CV_WINDOW_NORMAL);
    }
}
void cvdisp (const char** const names, const int count)
{
    for (int i=0;i<count;i++)
    {
        output_s out = getOutputByName(names[i]);
        const unsigned int size = (out.data.TA_data.size - (2*out.data.TA_data.offset))*out.data.TA_data.subgrid;
        Compute_float* data = taggedarrayTocomputearray(out.data.TA_data);
        PlotColored(out.name,data,out.data.TA_data.minval,out.data.TA_data.maxval,size);
        cvWaitKey(1);
        free(data);
    }
}
#endif
///Set up the outputtables for a given model
///@param m the model we are going to output stuff from
void output_init(const model* const m)
{
    //WHEN YOU ADD SOMETHING - INCREASE OUTPUT_COUNT AT TOP OF FILE;
    //ALSO - only add things to the end of the array
    output_s* outdata=(output_s[]){ //note - neat feature - missing elements initailized to 0
        //Name          data type                  actual data                size                    offset         subgrid,minval,maxval
        {"gE",          FLOAT_DATA, .data.TA_data={m->gE,                     conductance_array_size, couplerange,   1,0,2}}, //gE is a 'large' matrix - as it wraps around the edges
        {"gI",          FLOAT_DATA, .data.TA_data={m->gI,                     conductance_array_size, couplerange,   1,0,2}}, //gI is a 'large' matrix - as it wraps around the edges
        {"Coupling1",   FLOAT_DATA, .data.TA_data={m->layer1.connections,     couple_array_size,      0,             1,-0.5,0.5}}, //return the coupling matrix of layer 1 //TODO: fix min and max values
        {"Coupling2",   FLOAT_DATA, .data.TA_data={m->layer2.connections,     couple_array_size,      0,             1,-0.5,0.5}}, //return the coupling matrix of layer 2
        {"V1",          FLOAT_DATA, .data.TA_data={m->layer1.voltages_out,    grid_size,              0,             1,m->layer1.P->potential.Vin,m->layer1.P->potential.Vpk}},
        {"V2",          FLOAT_DATA, .data.TA_data={m->layer2.voltages_out,    grid_size,              0,             1,m->layer2.P->potential.Vin,m->layer2.P->potential.Vpk}},
        {"Recovery1",   FLOAT_DATA, .data.TA_data={m->layer1.recoverys_out,   grid_size,              0,             1,0,100}}, //TODO: ask adam for max and min recovery values
        {"Recovery2",   FLOAT_DATA, .data.TA_data={m->layer2.recoverys_out,   grid_size,              0,             1,0,100}}, //TODO: ask adam for max and min recovery values
        {"STDU1",       FLOAT_DATA, .data.TA_data={Features.STD==ON?m->layer1.std->U:NULL, grid_size, 0,             1,0,1}},
        {"STDR1",       FLOAT_DATA, .data.TA_data={Features.STD==ON?m->layer1.std->R:NULL, grid_size, 0,             1,0,1}},
        {"STDU2",       FLOAT_DATA, .data.TA_data={Features.STD==ON?m->layer2.std->U:NULL, grid_size, 0,             1,0,1}},
        {"STDR2",       FLOAT_DATA, .data.TA_data={Features.STD==ON?m->layer2.std->R:NULL, grid_size, 0,             1,0,1}},
        {"STDP1",       FLOAT_DATA, .data.TA_data={Features.STDP==ON?m->layer1.STDP_data->connections:NULL,grid_size,0,couple_array_size,-0.01,0.01}},
        {"STDP2",       FLOAT_DATA, .data.TA_data={Features.STDP==ON?m->layer2.STDP_data->connections:NULL,grid_size,0,couple_array_size,-0.01,0.01}},
        {"Spike1",      SPIKE_DATA, .data.Lag_data=&m->layer1.firinglags},
        {"Spike2",      SPIKE_DATA, .data.Lag_data=&m->layer2.firinglags},
        {.name={0}}};         //a marker that we are at the end of the outputabbles list
    output_s* malloced = malloc(sizeof(output_s)*output_count);
    memcpy(malloced,outdata,sizeof(output_s)*output_count);
    Outputtable = malloced;
}
///Cleans up memory and file handles that are used by the outputtables object
void CleanupOutput()
{
    for (int i=0;i<output_count;i++)
    {
        if (outfiles[i] != NULL)
        {
            fclose(outfiles[i]);
            outfiles[i]=NULL;
        }
    }
    free(Outputtable);//also cleanup outputtables
    Outputtable=NULL;
}
