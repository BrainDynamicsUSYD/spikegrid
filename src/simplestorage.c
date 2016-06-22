#include "simplestorage.h"

void AddnewSpike_simple(const size_t index,simplestorage* s)
{
    s->lags[index]=s->trefrac_in_ts;
}
