/// \file
typedef struct STDP_parameters STDP_parameters;
typedef struct ringbuffer ringbuffer;
void doSTDP (Compute_float* dmats,const ringbuffer* const fdata , const Compute_float*constm,const STDP_parameters S);
