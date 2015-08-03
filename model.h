/// \file
#include "layer.h" //TODO would be nice if this wasn't needed
#include "sizes.h"
typedef struct animal animal;
typedef struct condmat {
    Compute_float gE [conductance_array_size*conductance_array_size];
    Compute_float gI [conductance_array_size*conductance_array_size];
} condmat;
///Allows for having multiple layers and simulating them
///This holds both layes and things that we need to keep track of over time that aren't in the layers (main example gE,gI)
typedef struct model
{
	layer layer1;                                                       ///< First layer
	layer layer2;
    unsigned int timesteps;
    const LayerNumbers NoLayers;                                        ///<Whether this is a single or double layer model
	animal*         animal;
    ///Make these part of the struct to ensure they are nearby in memory - however it means you can't allocate a model on the stack
    condmat* const __restrict cond_matrices;
    Compute_float gIinit [conductance_array_size*conductance_array_size];   ///<gI matrix (large)
    Compute_float gEinit [conductance_array_size*conductance_array_size];   ///<gI matrix (large)

} model;
//void SaveModel(const model* const m);
