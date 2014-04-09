#ifndef HELPERS
#define HELPERS
#include "parameters.h"
//helper type for coordinates - try to use taher than passing around x,y pairs
typedef struct coords {int x;int y;} coords;
//store data in a ring - used for things like firing histories
typedef struct ringbuffer {
    coords ** data;
    unsigned int count;
    unsigned int curidx;
} ringbuffer;

coords* ringbuffer_getoffset (const ringbuffer* const input,const int offset);

//used for storing arrays with their size.  Allows for the matlab_output function to take both the big and large arrays
typedef struct {
    //we require volatile below as we don't want you to be able to write to an array using the pointer from the tagged array
    //however, other parts of the code could modify the underlying array, so use volatile to force reads
    const Compute_float* const volatile data;
    const unsigned int size;
    const unsigned int offset;
   } 
    tagged_array;
typedef struct STD_data
{   //some parts of this should be const - but oh well
    unsigned int ftimes[grid_size*grid_size];
    Compute_float U[grid_size*grid_size];
    Compute_float R[grid_size*grid_size];
    const STD_parameters* P;
} STD_data;
//hold the requisite data for a layer that enables it to be evolved through time.
typedef struct layer
{
    Compute_float* connections; //need some way to constify this.  Also - think up some efficient way to only add the points in a circle.
    Compute_float* STDP_connections;
    Compute_float voltages[grid_size*grid_size]; //possibly these should be pointers so that things can be copied in/out a bit faster.  Even better would probably be to move the v_out to somewhere else
    Compute_float voltages_out[grid_size*grid_size]; //return value - probably shouldn't be here
    const Compute_float* Extimecourse;  //store time course of Ex synapses  - need to make const
    const Compute_float* Intimecourse;  //store time course of In synapses  - need to make const
    ringbuffer spikes;
    STD_data std;
    const conductance_parameters* P;
    const STDP_parameters * S;
} layer_t;
//these break vim syntax highlighting so move to the end
//also - these don't look like C but are the official gcc-approved way of doing the max/min macros.  
//The typeof are required for the case of max (a++,b) which otherwise will be weird
#define max(a,b) \
    ({ __typeof__ (a) _a = (a);\
       __typeof__ (b) _b = (b); \
        _a>_b?_a:_b;})
#define min(a,b) \
    ({ __typeof__ (a) _a = (a);\
       __typeof__ (b) _b = (b); \
        _a<_b?_a:_b;})
#endif
