/// \file
#include "paramheader.h"
void randinit(Compute_float* input,const Compute_float minval, const Compute_float maxval, const unsigned int trialnum);
void Fixedinit(Compute_float* input, const Compute_float def_value,const Compute_float mod_value);
model* setup(const parameters p,const parameters p2, const LayerNumbers lcount,int jobnumber);
///contains the various outputtables - set in setup()
