#ifndef HELPERS
#define HELPERS
///helper type for coordinates/
///try to use this than passing around x,y pairs as it is simpler to understand.
///Computational overhead should be minimal
typedef struct coords {int x; /**<x coord*/int y;/**<y coord*/} coords;
///store data in a ring - used for things like firing histories
typedef struct ringbuffer {
    coords ** data;     ///<the actual data
    unsigned int count; ///<the total size of the ringbuffer
    unsigned int curidx;///<were we currently are in the ringbuffer
} ringbuffer;

coords* ringbuffer_getoffset (const ringbuffer* const input,const int offset);

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
