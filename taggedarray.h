#ifndef TAGGEDARRAY
#define TAGGEDARRAY

typedef struct {
    const float* const data;
    const int size;
    const int offset;} 
    tagged_array;
#endif
