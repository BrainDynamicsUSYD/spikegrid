/// \file
#ifndef LAYER
#define LAYER
#include "ringbuffer.h"
#include "paramheader.h"
///Holds data for STD on a per-neuon basis
typedef struct STD_data
{   //some parts of this should be const - but oh well
    unsigned int* ftimes;   ///< time of previous firing
    Compute_float* U;       ///< U STD parameter
    Compute_float* R;       ///< R STD parameter
} STD_data;
///hold the requisite data for a layer that enables it to be evolved through time.
typedef struct layer
{
    const Compute_float* const connections;     ///<Matrix of connections coming from a single point
    Compute_float* STDP_connections;            ///<Dynamic connections from STDP
    //problem here - these voltages aren't copied
    Compute_float* voltages;                    ///<Input voltages
    Compute_float* voltages_out;                ///<return value 
    const Compute_float* const Extimecourse;    ///<store time course of Ex synapses  
    const Compute_float* const Intimecourse;    ///<store time course of In synapses  
    ringbuffer spikes;                          ///<stores spiking history
    parameters* P;                              ///<The parameters that we used to make the layer
    STD_data std;                               ///<Some info that is needed for STD
} layer;
///Allows for having multiple layers and simulating them
typedef struct Model
{
    const LayerNumbers NoLayers;                                        ///<Whether this is a single or double layer model
    layer layer1;                                                       ///< First layer
    layer layer2;                                                       ///< Second layer
    ///Make these part of the struct to ensure they are nearby in memory - however it means you can't allocate a model on the stack
    Compute_float gE [conductance_array_size*conductance_array_size];   ///<gE matrix (large)
    Compute_float gI [conductance_array_size*conductance_array_size];   ///<gI matrix (large)
} model;
#endif
