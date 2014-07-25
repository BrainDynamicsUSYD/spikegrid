/// \file
#define PARAMETERS 
#include "whichparam.h"  
#undef  PARAMETERS
//get some macros for various sizes
///Size of the "large" arrays (notable examples are gE and gI)
#define conductance_array_size (grid_size + 2*couplerange)
///Size of a coupling matrix
#define couple_array_size (2*couplerange + 1)

