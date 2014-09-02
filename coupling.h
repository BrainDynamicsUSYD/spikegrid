/// \file
#include "typedefs.h"
typedef struct couple_parameters couple_parameters;
typedef struct decay_parameters decay_parameters;
Compute_float* CreateCouplingMatrix(const couple_parameters c);
unsigned int __attribute__((pure)) setcap(const decay_parameters d,const Compute_float minval, const Compute_float timestep);
Compute_float* __attribute__((const)) Synapse_timecourse_cache (const unsigned int cap, const decay_parameters Decay,const Compute_float timestep);

Compute_float  __attribute__((pure)) Synapse_timecourse(const decay_parameters Decay,const Compute_float time);

void Non_zerocouplings(const couple_parameters c,Compute_float** validconns,int* counts);
