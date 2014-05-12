#ifndef LAYER
#define LAYER
#include "ringbuffer.h"
#include "paramheader.h"
typedef struct STD_data
{   //some parts of this should be const - but oh well
    unsigned int ftimes[grid_size*grid_size];
    Compute_float U[grid_size*grid_size];
    Compute_float R[grid_size*grid_size];
} STD_data;
//hold the requisite data for a layer that enables it to be evolved through time.
typedef struct layer
{
    Compute_float* connections; //need some way to constify this.  Also - think up some efficient way to only add the points in a circle.
    Compute_float* STDP_connections;
    //problem here - these voltages aren't copied
    Compute_float voltages[grid_size*grid_size]; //possibly these should be pointers so that things can be copied in/out a bit faster.  Even better would probably be to move the v_out to somewhere else
    Compute_float voltages_out[grid_size*grid_size]; //return value - probably shouldn't be here
    const Compute_float* Extimecourse;  //store time course of Ex synapses  - need to make const
    const Compute_float* Intimecourse;  //store time course of In synapses  - need to make const
    ringbuffer spikes;
    parameters* P;
    STD_data std;
} layer;
///Allows for having multiple layers and simulating them
typedef struct Model
{
    const LayerNumbers NoLayers;
    layer layer1; ///< First layer
    layer layer2; ///< Second layer
    Compute_float gE [conductance_array_size*conductance_array_size]; ///<gE matrix (large)
    Compute_float gI [conductance_array_size*conductance_array_size]; ///<gI matrix (large)
} model;
#endif
