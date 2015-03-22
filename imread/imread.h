/// \file
#include "../typedefs.h"
#ifdef __cplusplus
	extern "C" {
#endif
#include "../cppparamheader.h"
typedef struct STDP_data STDP_data;
void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S,const Compute_float threshold, STDP_data* stdp);
int trialno ();
#ifdef __cplusplus
	}
#endif
