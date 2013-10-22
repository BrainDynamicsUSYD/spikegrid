#ifndef TAGGEDARRAY
#define TAGGEDARRAY
#include "parameters.h"
typedef struct {
    const Compute_float* const data;
    const int size;
    const int offset;} 
    tagged_array;
#endif
