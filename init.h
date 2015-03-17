/// \file
#include "paramheader.h" //needed for parameters
typedef struct model model;
void randinit(Compute_float* input,const Compute_float minval, const Compute_float maxval);
void Fixedinit(Compute_float* input, const Compute_float def_value,const Compute_float mod_value);
model* setup(const parameters p,const parameters p2, const LayerNumbers lcount,int jobnumber,const int yossarianjobnumber); //todo: why do we use straight parameters objects here.  This should be pointers - then we don't need to #include "paramheader.h"
