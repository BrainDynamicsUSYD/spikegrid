#include "typedefs.h"
typedef struct RD_data RD_data;
void evolvept_duallayer (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float strmod, Compute_float* __restrict condmat);
void evolvept_duallayer_STDP (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float* const __restrict STDP_connections,const Compute_float strmod, Compute_float* __restrict condmat);
long long int spikecount;

void AddRD (const coords c,const Compute_float* const __restrict connections, RD_data* __restrict RD, const Compute_float initstr);

void AddRD_STDP (const coords c,const Compute_float* const __restrict connections,const Compute_float* const __restrict STDP_connections, RD_data* __restrict RD,const Compute_float initstr);
void check();
