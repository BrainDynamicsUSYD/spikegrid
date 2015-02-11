/// \file
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "paramheader.h"
#include "mymath.h" //fabsf
#include "STDP.h"
#include "layer.h"
#include "randconns.h"
#include "lagstorage.h"
typedef struct
{
    Compute_float Strength_increase;
    Compute_float Strength_decrease;
    on_off        valid;
} STDP_change;

//gcc thinks that this can be const - but I think the read of timestep prevents this - not sure what is going on - maybe timestep gets inlined?
Compute_float __attribute__((pure)) STDP_strength(const STDP_parameters S,const Compute_float lag) //implicitrly assumed that the function is odd
{
    return  S.stdp_strength * exp(-lag*Features.Timestep/S.stdp_tau);
}
int __attribute__((pure,const)) wrap (int n)
{
    if (n<0) {return grid_size+n;}
    if (n>=grid_size) {return n-grid_size;}
    else {return n;}
}

inline static Compute_float clamp(Compute_float V,Compute_float target,Compute_float frac)
{
    target = fabs(target);
    if     (target > 1)        {printf("large target created\n");}
    if     (V > target * frac) {return(target * frac);}
    else if(V < target * -frac){return(target * -frac);}
    else   {return V;}

}

STDP_change STDP_change_calc (const int destneuronidx,const int destinotherlayeridx, const STDP_parameters S, const STDP_parameters S2,const int16_t* const lags,const int16_t* revlags)
{
    STDP_change ret = {.Strength_increase=Zero, .Strength_decrease=Zero,.valid=OFF};
    int idx=0;
    while (lags[destneuronidx+idx] != -1) //connections within the layer
    {
        ret.Strength_decrease += STDP_strength(S,lags[destneuronidx+idx]);
        ret.Strength_increase += STDP_strength(S,lags[destneuronidx+idx]);
        idx++;
        ret.valid = ON;
    }
    idx = 0;
    while (revlags[destinotherlayeridx+idx] != -1) //connections to the other layer.  This is way too complicated - but I tghink it should work
    {
        ret.Strength_increase += STDP_strength(S2,revlags[destinotherlayeridx+idx]);
        //the connection from the other layer to the current layer uses the other layer parameters
        //slightly arbitrary but feels correct and maintains the sum of STDP=0 when window function is odd.
        ret.Strength_decrease += STDP_strength(S, revlags[destinotherlayeridx+idx]);
        idx++;
    }
    return ret;
}

//invert a vector joining 2 points (not really - applied after there has been some additions and also does some tidying up)
inline static int invertdist(int v) {return ((2*couplerange) - v);}
void STDP_At_point(const int x, const int y,STDP_data* const data,STDP_data* const revdata,const STDP_parameters S,const STDP_parameters S2,const int xoffset,const int yoffset,
        const Compute_float* const const_couples,const Compute_float* const revconst_couples)
{
    const int wx = wrap(x+xoffset);
    const int wy = wrap(y+yoffset);
    //calculate the indexes for the neuron we are connected to and compute the offsets
    const int baseidx = LagIdx(wx,wy,data->lags);
    const int baseidx2 = LagIdx(wx,wy,revdata->lags);
    STDP_change change = STDP_change_calc(baseidx,baseidx2,S,S2,data->lags->lags,revdata->lags->lags);
    if (change.valid==ON)
    {
        // the other neuron actually fired - so we can apply STDP - need to apply in two directions
        //calculate the offsets
        const int cdx              = -xoffset + STDP_RANGE;
        const int cdy              = -yoffset + STDP_RANGE;
        const int rx               = invertdist(cdx);
        const int ry               = invertdist(cdy);
        const int fidx             = (wx*grid_size+wy)*STDP_array_size*STDP_array_size + cdx*STDP_array_size + cdy;
        const int ridx             = (x*grid_size+y)  *STDP_array_size*STDP_array_size + rx *STDP_array_size + ry;
        data->connections[fidx]    = clamp(data->   connections[fidx] + change.Strength_increase  ,const_couples[   cdx*couple_array_size + cdy],S.stdp_limit);
        data->connections[ridx]    = clamp(data->   connections[ridx] - change.Strength_decrease  ,const_couples[   cdx*couple_array_size + cdy],S.stdp_limit);
        revdata->connections[ridx] = clamp(revdata->connections[ridx] - change.Strength_increase  ,revconst_couples[cdx*couple_array_size + cdy],S2.stdp_limit);

        if (fabs(data->connections[fidx]) > 1.0 || fabs(data->connections[ridx]) > 1.0)
        {
          //  printf("something bad has happened at idx %i or %i %i %i\n",fidx,ridx,x,y);
        }
    }
}
void  DoSTDP(const Compute_float* const const_couples, const Compute_float* const const_couples2,
        STDP_data* data,const STDP_parameters S, STDP_data* const data2,const STDP_parameters S2,
        randconns_info* rcs)
{
    if (S.STDP_on ==OFF) {return;}
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            const int baseidx = LagIdx(x,y,data->lags);
            //first - check if the neuron has fired this timestep
            int idx = 0;
            while (data->lags->lags[baseidx + idx] != -1)
            {
                idx++; //we need to get to the last entry to ensure the neuron fired
            }
            if (idx >0 && data->lags->lags[baseidx+idx-1]==1) //so the neuron did actually fire at the last timestep
            {
                //now check if other neurons recently fired - first for nearby connections
                for (int i = -STDP_RANGE;i<=STDP_RANGE;i++)
                {
                    for (int j = -STDP_RANGE ;j<=STDP_RANGE;j++)
                    {
                        if (x+i<0 || x+i>=grid_size || y+j<0 || y+j>=grid_size || i*i+j*j > STDP_RANGE_SQUARED) {continue;}
                        STDP_At_point(x,y,data,data2,S,S2,i,j,const_couples,const_couples2);
                    }
                }
                if (Features.Random_connections == ON)
                {
                    unsigned int norand;
                    randomconnection* randconns = GetRandomConnsLeaving(x,y,*rcs,&norand);
                    //random connections away from (x,y) - these will be getting decreased
                    for (unsigned int i = 0;i<norand;i++)
                    {
                        const int destidx           = LagIdx(randconns[i].destination.x,randconns[i].destination.y,data->lags);
                        const int destidx2          = LagIdx(randconns[i].destination.x,randconns[i].destination.y,data2->lags);
                        STDP_change rcchange        = STDP_change_calc(destidx,destidx2,S,S2,data->lags->lags,data2->lags->lags);
                        randconns[i].stdp_strength  = clamp(randconns[i].stdp_strength-rcchange.Strength_decrease*500.0,randconns[i].strength,S.stdp_limit*1000.0);
                    }
                    //random connections to (x,y) - these will be getting increased - code is almost identical - except sign of change is reversed
                   unsigned int noconsArriving;
                   randomconnection** rcbase = GetRandomConnsArriving(x,y,*rcs,&noconsArriving);
                   for (unsigned int i=0;i<noconsArriving;i++)
                   {
                       randomconnection* rc = rcbase[i];
                       const int destidx    = LagIdx(rc->source.x,rc->source.y,data->lags);
                       const int destidx2   = LagIdx(rc->source.x,rc->source.y,data2->lags);
                       STDP_change rcchange = STDP_change_calc(destidx,destidx2,S,S2,data->lags->lags,data2->lags->lags);
                       rc->stdp_strength    = clamp(rc->stdp_strength+rcchange.Strength_decrease*500.0,rc->strength,S.stdp_limit*1000.0);
                       //                                             ^ note plus sign (not minus) why?? - I assume the strengths are reversed
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
        .lags = lagstorage_init(stdplagcount,STDP_cap),
        .connections =  calloc(sizeof(Compute_float),grid_size*grid_size*STDP_array_size*STDP_array_size)
    };
    memcpy(ret,&D,sizeof(*ret));
    return ret;
}

Compute_float* COMangle(const  STDP_data* const S)
{
    Compute_float* ret = malloc(sizeof(*ret)*grid_size*grid_size);
    for (int i=0;i<grid_size*grid_size;i++)
    {
        Compute_float xsum = Zero;
        Compute_float ysum = Zero;
        for (int a=-STDP_RANGE;a<STDP_RANGE;a++)
        {
            for (int b=-STDP_RANGE;b<STDP_RANGE;b++)
            {
                Compute_float str = S->connections[i*STDP_array_size*STDP_array_size+(a+STDP_RANGE)*STDP_array_size + (b+STDP_RANGE)];
                xsum += str*(Compute_float)a;
                ysum += str*(Compute_float)b;
            }
        }
        ret[i]=atan2(ysum,xsum);
    }
    return ret;
}
