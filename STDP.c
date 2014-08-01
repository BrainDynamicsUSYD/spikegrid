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
inline static Compute_float clamp(Compute_float V,Compute_float target,Compute_float frac)
{
    target = fabs(target);
    if (target > 1) {printf("large target created\n");}
    if (V > target * frac) {return (target * frac);}
    else if (V < target * -frac) {return (target * -frac);}
    else {return V;}

}
///invert a vector joining 2 points (not really - applied after there has been some additions and also does some tidying up)
inline static int invertdist(int v) {return ((2*couplerange) - v);}

void  DoSTDP(const Compute_float* const const_couples,STDP_data* data,const STDP_parameters S)
{
    if (S.stdp_strength ==Zero) {return;}
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
                        if (x+i<0 || x+i>=grid_size || y+j<0 || y+j>=grid_size || i*i+j*j > STDP_RANGE_SQUARED) {continue;}
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
                            const int ridx = (x*grid_size+y)*STDP_array_size*STDP_array_size + rx*STDP_array_size + ry;
               //             if (fidx < 0) {printf("small fidx error\n");}
                 //           if (fidx > grid_size*grid_size*STDP_array_size*STDP_array_size) {printf("large fidx error\n");}
                            data->connections[fidx] = clamp(data->connections[fidx] + str,const_couples[cdx*couple_array_size + cdy],S.stdp_limit);
                            data->connections[ridx] = clamp(data->connections[ridx] - str,const_couples[cdx*couple_array_size + cdy],S.stdp_limit);
                            if (fabs(data->connections[fidx]) > 1.0 || fabs(data->connections[ridx]) > 1.0)
                            {
                                printf("something bad has happened at idx %i or %i %i %i %i %i\n",fidx,ridx,x,y,i,j);
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
    STDP_data D = 
    {
        .lags = 
        {
            .lags = calloc(sizeof(int16_t),grid_size*grid_size*(size_t)stdplagcount),
            .cap  = STDP_cap,
            .lagsperpoint = stdplagcount
        },
        .connections =  calloc(sizeof(Compute_float),grid_size*grid_size*STDP_array_size*STDP_array_size)

    };    
    for (int x = 0;x<grid_size;x++)
    {
        for (int y = 0;y<grid_size;y++)
        {
            D.lags.lags[(x*grid_size+y)*D.lags.lagsperpoint]=-1;
        }
    }

    memcpy(ret,&D,sizeof(*ret));
    return ret;
}

