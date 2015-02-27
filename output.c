/// \file
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "STD.h"
#include "paramheader.h" //needed because some of the max/min values are obtained from parameters
#include "STDP.h"
#include "model.h"
#include "output.h"
#include "tagged_array.h"
///Total number of things to be output - occasionally needs to be incremented
#define output_count   19
#define overlay_count  3

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
///Finds an overlay which matches the given name - case sensitive
///@param name the name of the overlayt
overlaytext* __attribute__((pure)) getOverlayByName(const char* const name)
{
    int outidx=0;
    while (strlen(overlays[outidx].name) != 0)
    {
        if (!strcmp(overlays[outidx].name,name))
        {
            return &overlays[outidx];
        }
        outidx++;
    }
    return NULL;
}
int getTimeStep()
{
    return 1;
}

///Set up the outputtables for a given model
///This function should probably move to the C++ code
///@param m the model we are going to output stuff from
void output_init(const model* const m)
{
    //WHEN YOU ADD SOMETHING - INCREASE OUTPUT_COUNT AT TOP OF FILE;
    //ALSO - only add things to the end of the array
    output_s* outdata=(output_s[]){ //note - neat feature - missing elements initailized to 0
        //Name          data type                  actual data                size                    offset     8bz634
        //subgrid,minval,maxval
        {"gE",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->gE,                     conductance_array_size, couplerange,   1,0,2)}, //gE is a 'large' matrix - as it wraps around the edges
        {"gI",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->gI,                     conductance_array_size, couplerange,   1,0,2)}, //gI is a 'large' matrix - as it wraps around the edges
        {"Coupling1",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.connections,     couple_array_size,      0,             1,-0.5,0.5)}, //return the coupling matrix of layer 1 //TODO: fix min and max values
        {"Coupling2",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.connections,     couple_array_size,      0,             1,-0.5,0.5)}, //return the coupling matrix of layer 2
        {"V1",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.voltages_out,    grid_size,              0,             1,m->layer1.P->potential.Vin,m->layer1.P->potential.Vpk)},
        {"V2",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.voltages_out,    grid_size,              0,             1,m->layer2.P->potential.Vin,m->layer2.P->potential.Vpk)},
        {"Recovery1",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.recoverys_out,   grid_size,              0,             1,0,100)}, //TODO: ask adam for max and min recovery values
        {"Recovery2",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.recoverys_out,   grid_size,              0,             1,0,100)}, //TODO: ask adam for max and min recovery values
        {"STDU1",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer1.std->U:NULL, grid_size, 0,             1,0,1)},
        {"STDR1",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer1.std->R:NULL, grid_size, 0,             1,0,1)},
        {"STDU2",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer2.std->U:NULL, grid_size, 0,             1,0,1)},
        {"STDR2",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer2.std->R:NULL, grid_size, 0,             1,0,1)},
        {"STDP1",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STDP==ON?m->layer1.STDP_data->connections:NULL,grid_size,0,couple_array_size,-0.01,0.01)},
        {"STDP2",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STDP==ON?m->layer2.STDP_data->connections:NULL,grid_size,0,couple_array_size,-0.01,0.01)},
        {"Spike1",      SPIKE_DATA, .data.Lag_data=m->layer1.firinglags},
        {"Spike2",      SPIKE_DATA, .data.Lag_data=m->layer2.firinglags},
        {.name={0}}};         //a marker that we are at the end of the outputabbles list
    output_s* malloced = malloc(sizeof(output_s)*output_count);
    memcpy(malloced,outdata,sizeof(output_s)*output_count);
    Outputtable = malloced;
    overlaytext* overdata = (overlaytext[]){
        {"Trialno",({int Trialno() { return (int) (m->timesteps * Features.Timestep /m->layer1.P->Stim.timeperiod - m->layer1.P->Stim.PreconditioningTrials)  ;} Trialno; })}, //not correct - need to use millis conversion
        {"Timestep",({int Timestep() {return (int)m->timesteps;} Timestep;})}, //C has anonymous functions - WTF!!
        {.name={0}}
    };
    overlaytext* overmalloc = malloc(sizeof(overlaytext)*overlay_count);
    memcpy(overmalloc,overdata,sizeof(overlaytext)*overlay_count);
    overlays = overmalloc;
}
///Cleans up memory and file handles that are used by the outputtables object
void CleanupOutput()
{
    free(Outputtable);//also cleanup outputtables
    Outputtable=NULL;
}
