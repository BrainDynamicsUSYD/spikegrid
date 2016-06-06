#include "sizes.h"

typedef struct simplestorage
{
    uint8_t lags[grid_size*grid_size];
} simplestorage;

void AddnewSpike_simple(const size_t index,const uint8_t val,simplestorage* s);
