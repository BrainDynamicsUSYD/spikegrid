/// \file
///This file produces the correct math functions for the compute_float type we are using.
///The more common solution would be tgmath.h - however
///tgmath.h produces lots of fun warnings about float <-> double <-> complex conversion.
///So instead, we just do things manually.
#include <math.h>
#include <stdlib.h>
#include "typedefs.h"
#include "VS/VS/Winheader.h"
#ifdef FAST
#define exp(x)   expf(x)
#define fabs(x)  fabsf(x)
#define sin(x)   sinf(x)
#define ceil(x)  ceilf(x)
#define cos(x)   cosf(x)
#define round(x) roundf(x)
#endif
static inline Compute_float RandFloat()
{   /* don't need to cast random as int/double uses double division */
    return random() / (Compute_float) RAND_MAX;
}
