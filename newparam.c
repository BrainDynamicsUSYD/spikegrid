/// \file
#include <stdlib.h>
#include <string.h>
#include "paramheader.h"
///Modifies a parameters object in place using memset magic
///@param input     parameter to modify
///@param offset    offset into parameters object to change
///@param newdata   the data to copy in
///@param newcount  the size of the data to copy in
parameters* modparam(const parameters* input,const int offset, const void* newdata, const size_t newcount)
{
    parameters* paramnew = malloc(sizeof(parameters));
    memcpy(paramnew,input,sizeof(parameters));
    void* paramvoid = (void*)paramnew;
    memcpy(paramvoid + offset,newdata,newcount);
    return paramnew;
}

///Does the linear interpolation for a sweep.
///Parameters are self explanatory
Compute_float __attribute__((const)) nthvalue (const Compute_float min,const Compute_float max,const unsigned int count,const unsigned int n)
{
	if (count==0)
		{return min;}
    else
    	{return min+(max-min)*(Compute_float)n/(Compute_float)count;}
}

/// Gets the nth parameter in a sweep.
/// @param input The initial parameter to modify
/// @param sweep The thing we are sweeping over
/// @param n Which job we are (this is used to calculate the correct parameter value in the linear spacing)
parameters* __attribute__((const)) GetNthParam(const parameters input, const sweepable sweep,const unsigned int n)
{
    const Compute_float value = nthvalue(sweep.minval,sweep.maxval,sweep.count,n);
    return modparam(&input,sweep.offset,&value,sizeof(value));
}
