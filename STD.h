/// \file
#ifndef STDH
#define STDH
#include "typedefs.h"
typedef struct STD_parameters STD_parameters;
///Holds data for STD on a per-neuron basis
typedef struct STD_data
{   //some parts of this should be const - but oh well
    unsigned int* ftimes;   ///< time of previous firing
    Compute_float* U;       ///< U STD parameter
    Compute_float* R;       ///< R STD parameter
} STD_data;
STD_data* STD_init(const STD_parameters s);
Compute_float STD_str (const STD_parameters s, const int x, const int y,const unsigned int time,const unsigned int lag, STD_data* const d);
///calculation of STD strength.  In .h file for inlining (might not be required)
#endif
