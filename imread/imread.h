/// \file
#ifdef __cplusplus
	extern "C" {
#endif
#include "../cppparamheader.h"
typedef struct STDP_data STDP_data;
typedef struct randconns_info randconns_info;
void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float threshold, STDP_data* stdp,randconns_info* const rcinfo,const on_off);
void ApplyContinuousStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float Timestep,const Compute_float* Phimat);
int trialno ();
#ifdef __cplusplus
	}
#endif
