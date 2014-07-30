/// \file
#include <stdlib.h>
#include <string.h>
#include "paramheader.h"
#include "ringbuffer.h"
#include "mymath.h" //fabsf
#include "sizes.h"
#include "layer.h"
#include "STDP.h"
#include <stdio.h>
Compute_float __attribute__((pure)) STDP_strength(const STDP_parameters S,const Compute_float lag)
{  
    return  S.stdp_strength * exp(-lag*Features.Timestep/S.stdp_tau);
}

///helper function for STDP.  Calculates distance between two neurons, taking into account wrapping in the network
///interesting idea - in some cases I don't care about this wrapping and could cheat
inline static int dist(int cur,int prev)
{
    int dx = cur-prev;
    if (dx > (grid_size/2)) {return (dx - grid_size);}
    else if (dx < -(grid_size/2)) {return (dx + grid_size);}
    else {return dx;}
}

///invert a vector joining 2 points (not really - applied after there has been some additions and also does some tidying up)
inline static int invertdist(int v) {return ((2*couplerange) - v);}

void  DoSTDP(Compute_float* const_couples,STDP_data* data,const STDP_parameters S)
{

    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            const int baseidx = (x*grid_size + y)*data->lags.lagsperpoint;
            //first - check if the neuron has fired this timestep
            int idx = 0;
            while (data->lags.lags[baseidx + idx] != -1) 
            {
                idx++;
            }
            if (idx >0 && data->lags.lags[baseidx+idx-1]==1) //so the neuron did actually fire at the last timestep
            {
                //now check if other neurons recently fired - first for nearby connections
                for (int i = -STDP_RANGE;i<=STDP_RANGE;i++)
                {
                    for (int j = -STDP_RANGE ;j<=STDP_RANGE;j++)
                    {
                        if (x+i<0 || x+i>=grid_size || y+j<0 || y+j>=grid_size) {continue;}
                        int idx2=0;
                        const int baseidx2 = (x*grid_size + y) *data->lags.lagsperpoint;
                        Compute_float str = Zero;
                        while (data->lags.lags[baseidx2+idx2] != -1)
                        {
                            str += STDP_strength(S,data->lags.lags[baseidx2+idx2]);
                            idx2++;
                        }
                        if (idx2 > 0) 
                        {
                            // the other neuron actually fired - so we can apply STDP - need to apply in two directions
                            //calculate the offsets
                            const int px = x+i;
                            const int py = y+j;
                            const int cdx = i + STDP_RANGE;
                            const int cdy = j + STDP_RANGE;
                            const int rx  = invertdist(cdx);
                            const int ry  = invertdist(cdy);
                            const int fidx = (px*grid_size+py)*STDP_array_size*STDP_array_size + cdx*STDP_array_size + cdy;
                            if (fidx<0) {printf("%i %i %i %i\n",px,py,cdx,cdy);};
                            if (fidx>grid_size*grid_size*STDP_array_size*STDP_array_size) {printf("%i %i %i %i\n",px,py,cdx,cdy);};
                            if (data->connections[fidx] < fabs(S.stdp_limit * const_couples[cdx*couple_array_size + cdy]))
                            {
                                data->connections[fidx] += str;
                                data->connections[(x*grid_size+y)*STDP_array_size*STDP_array_size + rx*STDP_array_size+ry]-=str;
                            }
                        }
                    }
                }
            }
        }
    }
}
STDP_data* STDP_init(const STDP_parameters S,const int trefrac_in_ts)
{
    STDP_data* ret = malloc(sizeof(*ret));    
    const int STDP_cap = (int)(S.stdp_tau*5.0 / Features.Timestep);
    const int stdplagcount = (int)((STDP_cap/trefrac_in_ts)+2);

    ret->connections = calloc(sizeof(Compute_float),grid_size*grid_size*STDP_array_size*STDP_array_size);
    lagstorage L = 
    {
        .lags = calloc(sizeof(int16_t),grid_size*grid_size*(size_t)stdplagcount),
        .cap  = STDP_cap,
        .lagsperpoint = stdplagcount
    };    for (int x = 0;x<grid_size;x++)
    {
        for (int y = 0;y<grid_size;y++)
        {
            L.lags[(x*grid_size+y)*L.lagsperpoint]=-1;
        }
    }

    memcpy(&ret->lags,&L,sizeof(lagstorage));
    return ret;
}

