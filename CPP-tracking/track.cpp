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
Compute_float Vreset; //if this was real c++ this would probably be in a class
struct wave
{
    std::vector<fcoords> centers;
};
void Track_init(const Compute_float reset)
{
    Vreset=reset;
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
    for (Neuron_coord i=0;i<size;i++) //assume that there is no overlap
    {
        for (Neuron_coord j=0;j<size;j++) //assume that there is no overlap
        {
            if (vdata->data.TA_data->data[i*size+j]==Vreset) 
            {
                firingcoords.push_back((coords){i,j});
            }
        }
    }
    //need to assign points to waves - how best to express as cpp
    //basic F# algorithm - get 
    for (auto & element : firingcoords) {
    }

    //Then calc centers

    //then assign center to past wave

    //then print? or calc MSDS

}
