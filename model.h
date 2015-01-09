/// \file
#include "layer.h" //would be nice if this wasn't needed
#include "sizes.h"
///Allows for having multiple layers and simulating them
typedef struct model
{
    const LayerNumbers NoLayers;                                        ///<Whether this is a single or double layer model
    layer layer1;                                                       ///< First layer
    layer layer2;                                                       ///< Second layer
    ///Make these part of the struct to ensure they are nearby in memory - however it means you can't allocate a model on the stack
    Compute_float gE [conductance_array_size*conductance_array_size];   ///<gE matrix (large)
    Compute_float gI [conductance_array_size*conductance_array_size];   ///<gI matrix (large)
} model;
