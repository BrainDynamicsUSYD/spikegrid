#include "math.h"
#ifdef FAST
//tgmath.h produces lots of fun warnings about float <-> double <-> complex conversion.  As a result, this is the new solution
#define exp(x) expf(x)
#define fabs(x) fabsf(x)
#define sin(x) sinf(x)
#endif
