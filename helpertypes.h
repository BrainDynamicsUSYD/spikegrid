#ifndef HELPERS
#define HELPERS
#include "ringbuffer.h"
#include "parameters.h"
typedef struct coords {int x;int y;} coords;
RINGBUFFER_DEF(coords);

typedef struct {
    const Compute_float* volatile const data;
    const int size;
    const int offset;
   } 
    tagged_array;
typedef struct STD_data
{   //some parts of this should be const - but oh well
    int ftimes[grid_size*grid_size];
    Compute_float U[grid_size*grid_size];
    Compute_float R[grid_size*grid_size];
    const STD_parameters* P;
} STD_data;
typedef struct layer
{
    Compute_float* connections; //need some way to constify this.  Also - think up some efficient way to only add the points in a circle.
    Compute_float* STDP_connections;
    Compute_float voltages[grid_size*grid_size]; //possibly these should be pointers so that things can be copied in/out a bit faster.  Even better would probably be to move the v_out to somewhere else
    Compute_float voltages_out[grid_size*grid_size]; //return value - probably shouldn't be here
    const Compute_float* Extimecourse;  //store time course of Ex synapses  - need to make const
    const Compute_float* Intimecourse;  //store time course of In synapses  - need to make const
    coords_ringbuffer spikes;
    STD_data std;
    const potential_parameters* P;
} layer_t;
//these break vim syntax highlighting so move to the end
#define max(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a>_b?_a:_b;})
#define min(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a<_b?_a:_b;})
#endif
