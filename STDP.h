/// \file
#include "typedefs.h"
#include "enums.h"
typedef struct STDP_parameters STDP_parameters;
typedef struct randconns_info randconns_info;
typedef struct lagstorage lagstorage;
typedef struct STDP_data //this does actually need to go in a header as the image reading code switches STDP on and off
{
    lagstorage* lags;
    Compute_float* connections;
    on_off RecordSpikes;
} STDP_data;
void  DoSTDP(const Compute_float* const const_couples, const Compute_float* const const_couples2,
        STDP_data* data,const STDP_parameters S, STDP_data* const data2,const STDP_parameters S2,
        randconns_info* rcs);
STDP_data* STDP_init(const STDP_parameters S,const int trefrac_in_ts);
Compute_float* COMangle(const  STDP_data* const S);
