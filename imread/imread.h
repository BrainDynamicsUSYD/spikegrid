/// \file
#include "../typedefs.h"
#ifdef __cplusplus
	extern "C" {
#endif
#include "../enums.h" //fix - this shouldn't be needed - need to fix cppparamheader
#include "../cppparamheader.h"
void ApplyStim(Compute_float* voltsin,const Compute_float timemillis,const Stimulus_parameters S);
#ifdef __cplusplus
	}
#endif
