#ifndef HELPERS
#define HELPERS
#include "ringbuffer.h"
#include "parameters.h"
typedef struct coords {int x;int y;} coords;
RINGBUFFER_DEF(coords);
#define max(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a>_b?_a:_b;})
#define min(a,b) \
    ({ typeof (a) _a = (a);\
       typeof (b) _b = (b); \
        _a<_b?_a:_b;})
typedef struct {
    const Compute_float* const data;
    const int size;
    const int offset;} 
    tagged_array;
#endif
