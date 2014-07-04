/// \file
#ifndef HELPERS
#define HELPERS
#include <stdint.h>
///helper type for coordinates.
///try to use this than passing around x,y pairs as it is simpler to understand.
///Computational overhead should be minimal
///The struct uses int16_t for size.  Note, if you are using a grid size < 128, it would be possible to use chars.  Using a grid size < 255 with char is also possible but would require some more extensive changes to the code (currently -1 in a coord is used to mark the end of an array - this would need to be changed to the max of whatever integer type is in use
///TODO: typedef this struct to use an appropriate data type automatically based on the grid size.
typedef struct coords {int16_t x; /**<x coord*/int16_t y;/**<y coord*/} coords;
///store data in a ring - used for things like firing histories
typedef struct ringbuffer {
    coords ** data;     ///<the actual data
    unsigned int count; ///<the total size of the ringbuffer
    unsigned int curidx;///<were we currently are in the ringbuffer
} ringbuffer;

coords* ringbuffer_getoffset (const ringbuffer* const input,const int offset);

///Gcc approved type safe max definition
#define max(a,b) \
    ({ __typeof__ (a) _a = (a);\
       __typeof__ (b) _b = (b); \
        _a>_b?_a:_b;})
///Gcc approved type safe min definition
#define min(a,b) \
    ({ __typeof__ (a) _a = (a);\
       __typeof__ (b) _b = (b); \
        _a<_b?_a:_b;})
#endif
