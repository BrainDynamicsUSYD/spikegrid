/// \file
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "paramheader.h"
///this macro works - somehow - you probably don't want to know - essentially it reuses the name as a value of the sweepabletypes enum and the name of a property
#define TryGetVal(parent,name,paramtype,new,old) .name = name == paramtype ? new: old.parent.name
//Works by pure magic and #defines
/// Create a new, modified parameters object
/// @param input    intial parameters object (to be copied)
/// @param newval   The nbew value to insert
/// @param sweep    The parameter to change
parameters __attribute__((const,pure)) modparam (const parameters input, const Compute_float newval,const sweepabletypes sweep)
{
    return (parameters)
    {
        .couple= input.couple,
        .potential=
        {
            TryGetVal(potential,Vrt,sweep,newval,input),
            TryGetVal(potential,Vlk,sweep,newval,input),
            TryGetVal(potential,Vex,sweep,newval,input),
            TryGetVal(potential,Vin,sweep,newval,input),
            TryGetVal(potential,glk,sweep,newval,input),
        },
        .STDP = 
        {
            TryGetVal(STDP,stdp_limit,sweep,newval,input),
            TryGetVal(STDP,stdp_tau,sweep,newval,input),
            TryGetVal(STDP,stdp_strength,sweep,newval,input),
        },
        .STD        = input.STD,
        .Movie      = input.Movie,
        .theta      = input.theta
    };
}

///Does the linear interpolation for a sweep.
///Parameters are self explanatory
Compute_float __attribute__((const)) nthvalue (const Compute_float min,const Compute_float max,const unsigned int count,const unsigned int n)
{
    return min+(min-max)*(Compute_float)n/(Compute_float)count;
}

///This tests whether the mod param function correctly copies all fields.  Ideally, we could use random data but because of padding this is not possible.
///As a result, failure to copy a zero value will not trigger an error.  Be very careful with such values
void testmodparam(const parameters input)
{
    parameters paramnew = modparam(input,One,dummy);
    const int memcmp_res = memcmp(&paramnew,&input,sizeof(parameters));
    if (memcmp_res !=0)
    {
        printf ("modparam is broken - ensure that all fields are correctly copied to the new structure - error at %i  - may just be an error with padding\n",memcmp_res);
      //  exit(0);
    }
    else {printf ("modparam works\n");}
}

/// Gets the nth parameter in a sweep.
/// @param input The initial parameter to modify
/// @param sweep The thing we are sweeping over
/// @param n Which job we are (this is used to calculate the correct parameter value in the linear spacing)
parameters __attribute__((const)) GetNthParam(const parameters input, const sweepable sweep,const unsigned int n)
{
    const Compute_float value = nthvalue(sweep.minval,sweep.maxval,sweep.count,n);
    return modparam(input,value,sweep.type);
}
