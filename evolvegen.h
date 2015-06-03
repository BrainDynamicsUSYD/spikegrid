#include "typedefs.h"
void evolvept_duallayer (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float strmod, Compute_float* __restrict condmat);
void evolvept_duallayer_STDP (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float* const __restrict STDP_connections,const Compute_float strmod, Compute_float* __restrict condmat);
long long int c;

void AddRD (const int x,const int y,const Compute_float* const __restrict connections, Compute_float* __restrict Rmat,Compute_float* __restrict Dmat, const Compute_float R,const Compute_float D);

void AddRD_STDP (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float* const __restrict STDP_connections, Compute_float* __restrict Rmat, Compute_float* __restrict Dmat,const Compute_float R, const Compute_float D );
void check();
