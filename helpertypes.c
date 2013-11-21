#include "helpertypes.h"
coords* ringbuffer_getoffset (const ringbuffer* const input,const int offset)
{
    if (offset <= input->curidx) {return input->data[input->curidx - offset];}
    else {return input->data[input->curidx - offset + input->count];}
}
