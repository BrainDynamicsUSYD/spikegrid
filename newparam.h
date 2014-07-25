/// \file
typedef struct sweepable sweepable;
typedef struct parameters parameters;
parameters* __attribute__((const)) GetNthParam(const parameters input, const sweepable sweep,const unsigned int n);
