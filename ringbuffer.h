//This is a good candidate for using C++ here to create a generic class
#ifndef RINGBUFFER
#define RINGBUFFER
#define RINGBUFFER_DEF(A) typedef struct  \
{                               \
    A** data;                   \
    int count;                  \
    int curidx;                 \
} A##_ringbuffer;

#define RINGBUFFER_GETOFFSET(input,offset,out)  \
        typeof(input) _input=(input);           \
        if (offset<=_input.curidx)               \
        {out=_input.data[_input.curidx-offset];} \
        else                                    \
        {out=_input.data[_input.curidx-offset+_input.count];}
#endif
