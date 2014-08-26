/// \file
#include <stdlib.h>
#include "STD.h"
#include "mymath.h"
#include "paramheader.h"
///Initialise the STD parameters to their initial values.  
///Failing to call this before using STD will give incorrect results initially.
///It is possible that results will converge after some time, but best not to risk it.
/// @param s the STD parameters
STD_data* __attribute__((const)) STD_init(const STD_parameters s)
{
    STD_data* ret = malloc(sizeof(STD_data)) ;
    ret->U=malloc(sizeof(Compute_float)*grid_size*grid_size);
    ret->R=malloc(sizeof(Compute_float)*grid_size*grid_size);
    ret->ftimes=malloc(sizeof(unsigned int)*grid_size*grid_size);
    for(int i=0;i < grid_size*grid_size;i++)
    {
        ret->ftimes[i]=0; //probably not required as you can gurantee that the memory is set to 0 by default
        ret->U[i] = s.U;
        ret->R[i] = One;
    }
    return ret;
}
Compute_float STD_str (const STD_parameters s, const int x, const int y,const unsigned int time,const int16_t lag, STD_data* const d)
{
    const int stdidx=x*grid_size+y;
    if (lag==1) //recalculate after spiking
    {
        const Compute_float spike_interval = ((Compute_float)(time-(d->ftimes[stdidx])))/((Compute_float)1000.0)*Features.Timestep;//calculate inter spike interval in seconds
        d->ftimes[stdidx]=time; //update the time
        const Compute_float prevu=d->U[stdidx]; //need the previous U value
        //newU = U + oldU*(1-U)*exp(-dt/F)
        d->U[stdidx] = s.U + d->U[stdidx]*(One- s.U)*exp(-spike_interval/s.F);
        //newR = 1 + (oldR-oldU*oldR-1)*exp(-dt/D)
        d->R[stdidx] = One + (d->R[stdidx] - prevu*d->R[stdidx] - One)*exp(-spike_interval/s.D);
    }
    return d->U[stdidx] * d->R[stdidx] * (1.0/s.U); //multiply by 1/u so that the initial spike has the correct size
}

