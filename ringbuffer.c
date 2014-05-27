/// \file
#include "ringbuffer.h"
///get an element from the ringbuffer with a given offset
/// @param input  input ringbuffer
/// @param offset offset from current index (i.e. how many elements backward in time to go)
coords* __attribute__((pure)) ringbuffer_getoffset (const ringbuffer* const input,const int offset)
{
    if (offset <=(int) input->curidx) {return input->data[(int)input->curidx - offset];}
    else {return input->data[(int)input->curidx - offset + (int)input->count];}
}
