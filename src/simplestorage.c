#include "simplestorage.h"

void AddnewSpike_simple(const size_t index,const uint8_t val,simplestorage* s)
{
    s->lags[index]=val;
}

