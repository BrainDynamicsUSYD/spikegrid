/// \file
#include "typedefs.h"
#include "enums.h"
typedef struct parameters paramaters;
void randinit(Compute_float* input,const Compute_float minval, const Compute_float maxval);
void Fixedinit(Compute_float* input, const Compute_float def_value,const Compute_float mod_value);
model* setup(const parameters p,const parameters p2, const LayerNumbers lcount,int jobnumber);
///contains the various outputtables - set in setup()
