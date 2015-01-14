/// \file
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "paramheader.h"
#include "mymath.h" //fabsf
#include "STDP.h"
#include "layer.h"
#include "sizes.h"
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

///helper function for STDP.  Calculates distance between two neurons, taking into account wrapping in the network
///interesting idea - in some cases I don't care about this wrapping and could cheat
inline static int dist(int cur,int prev)
{
    int dx = cur-prev;
    if     (dx >       (grid_size/2)){return(dx - grid_size);}
    else if(dx < -     (grid_size/2)){return(dx + grid_size);}
    else   {return dx;}
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
        ret.Strength_increase += STDP_strength(S2,revlags[destneuronidx+idx]);
        //the connection from the other layer to the current layer uses the other layer parameters
        //slightly arbitrary but feels correct and maintains the sum of STDP=0 when window function is odd.
        ret.Strength_decrease += STDP_strength(S, revlags[destneuronidx+idx]); 
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
    const int baseidx = (wx*grid_size + wy) * data->lags.lagsperpoint;
    const int baseidx2 = (wx*grid_size + wy) * revdata->lags.lagsperpoint;
    STDP_change change = STDP_change_calc(baseidx,baseidx2,S,S2,data->lags.lags,revdata->lags.lags);
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
        randconns_info* rcs,const randconn_parameters* const rparams )
{
    if (S.STDP_on ==OFF) {return;}
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            const int baseidx = (x*grid_size + y)*data->lags.lagsperpoint;
            //first - check if the neuron has fired this timestep
            int idx = 0;
            while (data->lags.lags[baseidx + idx] != -1)
            {
                idx++; //we need to get to the last entry to ensure the neuron fired
            }
            if (idx >0 && data->lags.lags[baseidx+idx-1]==1) //so the neuron did actually fire at the last timestep
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
                    randomconnection* randconns = GetRandomConnsLeaving(x,y,*rcs,rparams,&norand);
                    //random connections away from (x,y) - these will be getting decreased
                    for (unsigned int i = 0;i<norand;i++)
                    {
                        const int destidx  = ((randconns[i].destination.x * grid_size) + y)*data->lags.lagsperpoint;
                        const int destidx2 = ((randconns[i].destination.x * grid_size) + y)*data2->lags.lagsperpoint;
                        STDP_change rcchange   = STDP_change_calc(destidx,destidx2,S,S2,data->lags.lags,data2->lags.lags);
                        randconns[i].stdp_strength       = clamp(randconns[i].stdp_strength-rcchange.Strength_decrease,randconns[i].strength,S.stdp_limit);
                    }
                    //random connections to (x,y) - these will be getting increased - code is almost identical - except sign of change is reversed
                   randomconnection** rcbase = rcs->randconns_reverse_lookup[x*grid_size+y];
                   for (unsigned int i=0;i<rcs->rev_pp[x*grid_size+y];i++)
                   {
                       randomconnection rc = *rcbase[i];
                       const int destidx  = ((rc.destination.x * grid_size) + y)*data->lags.lagsperpoint;
                       const int destidx2 = ((rc.destination.x * grid_size) + y)*data2->lags.lagsperpoint;
                       STDP_change rcchange   = STDP_change_calc(destidx,destidx2,S,S2,data->lags.lags,data2->lags.lags);
                       rc.stdp_strength       = clamp(rc.stdp_strength+rcchange.Strength_increase,rc.strength,S.stdp_limit);
                       //                                             ^ note plus sign (not minus)
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
            .lags         = calloc(sizeof(int16_t),grid_size*grid_size*(size_t)stdplagcount),
            .cap          = STDP_cap,
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
