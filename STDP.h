/// \file
#include "layer.h"
typedef struct STDP_parameters STDP_parameters;
typedef struct ringbuffer ringbuffer;
typedef struct STDP_data
{
    lagstorage lags;
    Compute_float* connections;
} STDP_data;

void DoSTDP(Compute_float* const_couples,STDP_data* data,const STDP_parameters S);
STDP_data* STDP_init(const STDP_parameters S,const int trefrac_in_ts);
