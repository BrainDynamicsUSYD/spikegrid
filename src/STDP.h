/// \file
#ifndef STDP_FILE
#define STDP_FILE
#include "typedefs.h"
#include "enums.h"
typedef struct STDP_parameters STDP_parameters;
typedef struct randconns_info randconns_info;
typedef struct lagstorage lagstorage;
typedef struct tagged_array tagged_array;
typedef struct STDP_data //this does actually need to go in a header as the image reading code switches STDP on and off
{
    lagstorage* lags;
    Compute_float* connections;
    on_off RecordSpikes;
    const STDP_parameters* const P;
} STDP_data;
void  DoSTDP(const Compute_float* const const_couples, const Compute_float* const const_couples2,
        STDP_data* data, STDP_data* const data2,
        randconns_info* rcs, randconns_info* rcs2
        );
STDP_data* STDP_init(const STDP_parameters* const S,const int trefrac_in_ts);
Compute_float* COMangle(const  STDP_data* const S);

void STDP_decay(const  STDP_data* const S,randconns_info* rcs);
tagged_array* STDP_mag(const tagged_array* const in);
#ifdef TESTING //for unit testing the code
int __attribute__((pure,const)) wrap (int n);
Compute_float clamp(Compute_float V,Compute_float target,Compute_float frac);
#endif //note testing code
#endif
