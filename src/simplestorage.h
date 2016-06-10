#include "sizes.h"

typedef struct simplestorage
{
    uint8_t lags[grid_size*grid_size];
    uint8_t trefrac_in_ts; //TODO - figure out how to make this const
} simplestorage;

void AddnewSpike_simple(const size_t index,simplestorage* s);
