#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "paramheader.h"
//this macro works - somehow - you probably don't want to know
#define TryGetVal(parent,name,paramtype,new,old) .name = name == paramtype ? new: old.parent.name
//Works by pure magic and #defines
parameters __attribute__((const,pure)) modparam (const parameters input, const Compute_float newval,const sweepabletypes sweep)
{
    return (parameters)
    {
        .time={TryGetVal(time,dt,sweep,newval,input)},
        .couple=
        {
            TryGetVal(couple,WE,sweep,newval,input),
            TryGetVal(couple,sigE,sweep,newval,input),
            TryGetVal(couple,WI,sweep,newval,input),
            TryGetVal(couple,sigI,sweep,newval,input),
            TryGetVal(couple,SE,sweep,newval,input),
            TryGetVal(couple,SI,sweep,newval,input),
            .norm_type=input.couple.norm_type
        },
        .synapse = input.synapse,
        .potential=
        {
            TryGetVal(potential,Vrt,sweep,newval,input),
            TryGetVal(potential,Vth,sweep,newval,input),
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
        .features   = input.features,
        .theta      = input.theta
    };
}
Compute_float __attribute__((const)) nthvalue (const Compute_float min,const Compute_float max,const unsigned int count,const unsigned int n)
{
    return min+(min-max)*(Compute_float)n/(Compute_float)count;
}
//caller needs to free
parameters* CreateParamlist (const parameters input, const sweepable sweep)
{
    parameters* ret = calloc(sizeof(parameters),sweep.count); //need to use calloc to ensure that my test method works even when padding is present
    for (unsigned int i=0;i<sweep.count;i++)
    {
        const parameters newval = modparam(input,nthvalue(sweep.minval,sweep.maxval,sweep.count,i),sweep.type);
        memcpy(&(ret[i]),&newval,sizeof(parameters));
    }
    return ret;
}

//This tests whether the mod param function correctly copies all fields.  Ideally, we could use random data but because of padding this is not possible.
//As a result, failure to copy a zero value will not trigger an error.  Be very careful with such values
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
//get an array of parameters that vary according to a sweepable object
//caller needs to free
parameters* GetParamArray(const parameters input, const sweepable sweep)
{
    return CreateParamlist(input,sweep);
}

//Gets the nth parameter in a sweep
parameters GetNthParam(const parameters input, const sweepable sweep,const int n)
{
    return GetParamArray(input,sweep)[n];
}
