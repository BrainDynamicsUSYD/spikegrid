#ifndef OUTPUT
#define OUTPUT
#include "matlab_includes.h"
#include "pixeltypes.h"
#include "taggedarray.h"
typedef struct {
    char* name;
    tagged_array data;
} output_s;
bitmap_t* FloattoBitmap(const tagged_array input,const Compute_float maxval, const Compute_float minval);
#ifdef MATLAB
mxArray* outputToMxArray (const tagged_array input); 
#endif //matlab
#endif //OUTPUT
