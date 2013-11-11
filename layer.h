#ifndef LAYER
#define LAYER
#include "parameters.h"
#include "ringbuffer.h"
#include "helpertypes.h"
typedef struct layer
{
    Compute_float* connections; //need some way to constify this.  Also - think up some efficient way to only add the points in a circle.
    Compute_float* STDP_connections;
    Compute_float voltages[grid_size*grid_size]; //possibly these should be pointers so that things can be copied in/out a bit faster.  Even better would probably be to move the v_out to somewhere else
    Compute_float voltages_out[grid_size*grid_size];
    coords_ringbuffer spikes;
} layer_t;
#endif
