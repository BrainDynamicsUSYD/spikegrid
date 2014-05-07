#include "helpertypes.h"
#include <stdlib.h>
#include <string.h>
//When using a ringbuffer get the nth value from the input index
coords* __attribute__((pure)) ringbuffer_getoffset (const ringbuffer* const input,const int offset)
{
    if (offset <=(int) input->curidx) {return input->data[(int)input->curidx - offset];}
    else {return input->data[(int)input->curidx - offset + (int)input->count];}
}
void* newdata(const void* const input,const unsigned int size)
{
        void* ret = malloc(size);
            memcpy(ret,input,size);
                return ret;
} 
