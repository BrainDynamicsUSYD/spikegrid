#ifndef OUTPUT
#define OUTPUT
#include "matlab_includes.h"
#include "pixeltypes.h"
#include "helpertypes.h"
typedef struct {
    const char* const name;
    const tagged_array data;
} output_s; //used so that matlab has string identifiers that correspond to a specific tagged_array
bitmap_t* FloattoBitmap(const tagged_array input,const Compute_float maxval, const Compute_float minval);
#ifdef MATLAB
mxArray* outputToMxArray (const tagged_array input); 
#endif //matlab
#endif //OUTPUT
