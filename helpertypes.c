#include "helpertypes.h"
coords* __attribute__((pure)) ringbuffer_getoffset (const ringbuffer* const input,const int offset)
{
    if (offset <=(int) input->curidx) {return input->data[(int)input->curidx - offset];}
    else {return input->data[(int)input->curidx - offset + (int)input->count];}
}
