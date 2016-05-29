/// \file
typedef struct sweepable sweepable;
typedef struct parameters parameters;
#ifdef _WIN32
#include "VS\VS\Winheader.h"
#endif
parameters* __attribute__((const)) GetNthParam(const parameters input, const sweepable sweep,const unsigned int n);
