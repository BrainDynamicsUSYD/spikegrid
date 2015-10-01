/// \file
//track inputs
#include <iostream>
#include <vector>
extern "C"
{
#include "../cppparamheader.h"
#include "../output.h"
#include "../tagged_array.h"
}
//TODO: this file has a mix of files corresponding to 
Compute_float Vreset; //if this was real c++ this would probably be in a class
struct wave
{
    std::vector<fcoords> centers;
};
void Track_init(const Compute_float reset)
{
    Vreset=reset;
}

int fix_one_dimension(const int a, const int dx)
{
    if (a+dx>grid_size) {return (a+dx-grid_size);}
    else if (a+dx<0) {return (a+dx+grid_size);}
    else {return a+dx;}
}

coords movecoord(coords c,const int dx,const int dy,const int size)
{
    return (coords){.x=fix_one_dimension(c.x,dx),.y=fix_one_dimension(c.y,dy)};
}

const int waverange=3;
void markneighbours(coords c,const int waveno,int* firestatus)
{
    firestatus[grid_index(c)]=waveno;
    for (int i=-waverange;i<=waverange;i++)
    {
        for (int j=-waverange;j<=waverange;j++)
        {
            if (i != 0 && j != 0) //need to do this to avoid infinite recursion
            {
                const coords newcoord=movecoord(c,i,j);
            }
        }
    }
}

void Track(const output_s* const vdata)
{
    if (vdata->name[0] != 'V' || vdata->name[3] != 0) //make sure output is V1/V2.
    {
        std::cout << "Track called without being given a voltage input" << std::endl;
        return;
    }
    std::vector<coords> firingcoords;
    unsigned int size = tagged_array_size_(*vdata->data.TA_data);
    //now iterate over the neurons and find the firing neurons - use refractory to get smoothing
    //One other approach would be to use an array of coordinates and use firing rather than
    int* firestatus = calloc(sizeof(int),size*size);
    //initialise firing neurons to -1, otherwise 0.  When we group them, they will get a number for the group.
    for (int i=0;i<size*size;i++) //assume that there is no overlap
    {
        if (vdata->data.TA_data->data[i]==Vreset) 
        {
            firestatus[i]= (-1);
        }
    }
    //need to assign points to waves - how best to express as cpp
    //we iterate over all points
    int currentWaveNo=0;//starts as 0 - increment when first group is found.
    std::vector<std::vector<coords>>;//holds the list of waves
    for (int i=0;i<size*size;i++)
    {
        if (firestatus[i]== -1)//find a neuron that we recorded as spiking but haven't put in to a pattern yet.
        {
            //mark this wave - put in separate function for recursion.
            markneighbours(coords(i),currentWaveNo,firestatus);
        }
    }
    //Then calc centers

    //then assign center to past wave

    //then print? or calc MSDS

}
