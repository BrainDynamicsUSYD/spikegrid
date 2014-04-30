#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "paramheader.h"
#include "newparam.h"
//this macro works - somehow - you probably don't want to know
#define TryGetVal(parent,name,paramtype,new,old) .name = name == paramtype ? new: old.parent.name
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
            .rate=input.potential.rate
        },
        .STDP = 
        {
            TryGetVal(STDP,stdp_limit,sweep,newval,input),
            TryGetVal(STDP,stdp_tau,sweep,newval,input),
            TryGetVal(STDP,stdp_strength,sweep,newval,input),
        },
        .STD = input.STD,
        .Movie = input.Movie,
      //  .features = input.features,
    };
}

parameters* CreateParamlist (const parameters input, const float* list,unsigned int count,const sweepabletypes sweep)
{
    parameters* ret = calloc(sizeof(parameters),count); //need to use calloc to ensure that my test method works even when padding is present
    for (unsigned int i=0;i<count;i++)
    {
        const parameters newval = modparam(input,list[i],sweep);
        memcpy(&(ret[i]),&newval,sizeof(parameters));
    }
    return ret;

}
//caller needs to free memory
Compute_float* makevalues (const Compute_float min,const Compute_float max,const unsigned int count)
{
    Compute_float* ret = calloc(sizeof(Compute_float),count);
    for (unsigned int i=0;i<count;i++)
    {
        ret[i]=min+(min-max)*(Compute_float)i/(Compute_float)count;
    }
    return ret;
}
//This tests whether the mod param function correctly copies all fields.  Ideally, we could use random data but because of padding this is not possible.
//As a result, failure to copy a zero value will not trigger an error.  Be very careful with such values
void testmodparam(const parameters input)
{
    parameters paramnew = modparam(input,One,dummy);
    if (memcmp(&paramnew,&input,sizeof(parameters))!=0)
    {
        printf ("modparam is broken - ensure that all fields are correctly copied to the new structure\n");
        exit(0);
    }
    else {printf ("modparam works\n");}
}

parameters* GetParamArray(const parameters input, const sweepable sweep)
{
    const float* values = makevalues(sweep.minval,sweep.maxval,sweep.count);
    return CreateParamlist(input,values,sweep.count,sweep.type);
}
