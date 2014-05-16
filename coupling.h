/// \file
#include "paramheader.h"
Compute_float* CreateCouplingMatrix(const couple_parameters c);
unsigned int __attribute__((pure)) setcap(const decay_parameters d,const Compute_float minval, const Compute_float timestep);
Compute_float* __attribute__((const)) Synapse_timecourse_cache (const unsigned int cap, const decay_parameters Decay,const Compute_float timestep);
