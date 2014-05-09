#ifndef HELPERS
#define HELPERS
#include "paramheader.h"
//helper type for coordinates - try to use taher than passing around x,y pairs
typedef struct coords {int x;int y;} coords;
//store data in a ring - used for things like firing histories
typedef struct ringbuffer {
    coords ** data;
    unsigned int count;
    unsigned int curidx;
} ringbuffer;

coords* ringbuffer_getoffset (const ringbuffer* const input,const int offset);
void* newdata(const void* const input,const unsigned int size);


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
