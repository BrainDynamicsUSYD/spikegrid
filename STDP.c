;/// \file
#include <string.h>
#include <stdio.h>
#include "paramheader.h"
#include "mymath.h" //fabsf
#include "STDP.h"
#include "randconns.h"
#include "lagstorage.h"
#include "tagged_array.h"
typedef struct
{
    Compute_float Strength_increase;
    Compute_float Strength_decrease;
    on_off        valid;
} STDP_change;

//gcc thinks that this can be const - but I think the read of timestep prevents this - not sure what is going on - maybe timestep gets inlined?
Compute_float __attribute__((pure)) STDP_strength(const STDP_parameters* const S,const Compute_float lag) //implicitrly assumed that the function is odd
{
    return  S->stdp_strength * exp(-lag*Features.Timestep/S->stdp_tau);
}
//this takes a coordinate and maps it to the grid allowing for overflow over the edges.
int __attribute__((pure,const)) wrap (int n)
{
    if (n<0) {return grid_size+n;}
    if (n>=grid_size) {return n-grid_size;}
    else {return n;}
}

Compute_float clamp(Compute_float V,Compute_float target,Compute_float frac)
{
    target = fabs(target);
    if     (target > 1)        {printf("large target created\n");}
    if     (V > target * frac) {return(target * frac);}
    else if(V < target * -frac){return(target * -frac);}
    else   {return V;}
}

STDP_change STDP_change_calc (const unsigned int destneuronidx,const unsigned int destinotherlayeridx, const STDP_parameters* const S, const STDP_parameters * const S2,const int16_t* const lags,const int16_t* revlags)
{
    STDP_change ret = {.Strength_increase=Zero, .Strength_decrease=Zero,.valid=OFF};
    unsigned int idx=0;
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

//invert a vector joining 2 points (not really - applied after there has been some additions and also does some tidying up) - the inverse 
inline static int invertdist(int v) {return ((2*couplerange) - v);}
void STDP_At_point(const int x, const int y,STDP_data* const data,STDP_data* const revdata,const int xoffset,const int yoffset,
        const Compute_float* const const_couples,const Compute_float* const revconst_couples)
{
    const int wx = wrap(x+xoffset);
    const int wy = wrap(y+yoffset);
    //calculate the indexes for the neuron we are connected to and compute the offsets
    const unsigned int baseidx = LagIdx((unsigned)wx,(unsigned)wy,data->lags);
    const unsigned int baseidx2 = LagIdx((unsigned)wx,(unsigned)wy,revdata->lags);
    STDP_change change = STDP_change_calc(baseidx,baseidx2,data->P,revdata->P,data->lags->lags,revdata->lags->lags);
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
        data->connections[fidx]    = clamp(data->   connections[fidx] + change.Strength_increase  ,const_couples[   cdx*couple_array_size + cdy],data->P->stdp_limit);
        data->connections[ridx]    = clamp(data->   connections[ridx] - change.Strength_decrease  ,const_couples[   cdx*couple_array_size + cdy],data->P->stdp_limit);
        revdata->connections[ridx] = clamp(revdata->connections[ridx] - change.Strength_increase  ,revconst_couples[cdx*couple_array_size + cdy],revdata->P->stdp_limit);

        if (fabs(data->connections[fidx]) > 1.0 || fabs(data->connections[ridx]) > 1.0) //check is some connection has become massive - often indicative of a bug - note with strong STDP it is not necersarrily a bug.
        {
          //  printf("something bad has happened at idx %i or %i %i %i\n",fidx,ridx,x,y);
        }
    }
}
void  DoSTDP(const Compute_float* const const_couples, const Compute_float* const const_couples2,
        STDP_data* data, STDP_data* const data2,
        randconns_info* rcs)
{
    //whilst this looks like O(n^4) (n=gridsize), it is actually O(n^2) as STDP_RANGE is always fixed
    if (data->P->STDP_on ==OFF) {return;}
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            const unsigned int baseidx = LagIdx((unsigned)x,(unsigned)y,data->lags);
            //first - check if the neuron has fired this timestep
            if ( CurrentShortestLag(data->lags,baseidx)==1) //so the neuron did actually fire at the last timestep
            {
                //now check if other neurons recently fired - first for nearby connections
                for (int i = -STDP_RANGE;i<=STDP_RANGE;i++)
                {
                    for (int j = -STDP_RANGE ;j<=STDP_RANGE;j++)
                    {
                        if (i*i+j*j > STDP_RANGE_SQUARED) {continue;}
                        STDP_At_point(x,y,data,data2,i,j,const_couples,const_couples2);
                    }
                }
                if (Features.Random_connections == ON)
                {
                    unsigned int norand;
                    randomconnection* randconns = GetRandomConnsLeaving((unsigned)x,(unsigned)y,*rcs,&norand);
                    //random connections away from (x,y) - these will be getting decreased
                    for (unsigned int i = 0;i<norand;i++)
                    {
                        const unsigned int destidx           = LagIdx(randconns[i].destination.x,randconns[i].destination.y,data->lags);
                        const unsigned int destidx2          = LagIdx(randconns[i].destination.x,randconns[i].destination.y,data2->lags);
                        STDP_change rcchange        = STDP_change_calc(destidx,destidx2,data->P,data2->P,data->lags->lags,data2->lags->lags);
                        randconns[i].stdp_strength  = clamp(randconns[i].stdp_strength-rcchange.Strength_decrease,randconns[i].strength,data->P->stdp_limit);
                    }
                    //random connections to (x,y) - these will be getting increased - code is almost identical - except sign of change is reversed
                   unsigned int noconsArriving;
                   randomconnection** rcbase = GetRandomConnsArriving(x,y,*rcs,&noconsArriving);
                   for (unsigned int i=0;i<noconsArriving;i++)
                   {
                       randomconnection* rc = rcbase[i];
                       const unsigned int destidx    = LagIdx(rc->source.x,rc->source.y,data->lags);
                       const unsigned int destidx2   = LagIdx(rc->source.x,rc->source.y,data2->lags);
                       STDP_change rcchange = STDP_change_calc(destidx,destidx2,data->P,data2->P,data->lags->lags,data2->lags->lags);
                       rc->stdp_strength    = clamp(rc->stdp_strength+rcchange.Strength_decrease,rc->strength,data->P->stdp_limit);
                       //                                             ^ note plus sign (not minus) why?? - I assume the strengths are reversed - maybe this should be stremgth_increase?
                   }
                }
            }
        }
    }
}
//Look - we can write constructors for objects in C
STDP_data* STDP_init(const STDP_parameters* const S,const int trefrac_in_ts)
{
    STDP_data* ret = malloc(sizeof(*ret));
    const int STDP_cap = (int)(S->stdp_tau*5.0 / Features.Timestep); //massive hack - potential segfaults / errors when high firing rates and low tref
    const unsigned int stdplagcount = (unsigned int)((STDP_cap/trefrac_in_ts)+2); //int division is fine here
    STDP_data D =
    {
        .lags = lagstorage_init(stdplagcount,STDP_cap), //in the normal model, the lagstorage uses all the ram.  However, in the STDP code, the couplings use it all, so while we could do some trickery to avoid the duplicate lagstorages, it is not a huge issue - except we are doing some unnecersarry work here.  So it might be possible to eliminate this to improve performance.
        .connections =  calloc(sizeof(Compute_float),grid_size*grid_size*STDP_array_size*STDP_array_size), //This is a truly epic sized matrix.  For example a 300x300 grid with a coupling range of 25 will take up 1.7GB/layer.  Unfortuneately, I suspect that reductions in this size may cause perf problems.  Regardless we have 6GB/core on yossarian, so this is still OK.  Although maybe this calculation should use `long` to avoid overflow.
        //roughly 20% of the ram here can be saved if we don't store the zero entries.  Bigger saving would be in eliminating inactive neurons that would be 50%
        .RecordSpikes = ON,
        .P = S,
    };
    memcpy(ret,&D,sizeof(*ret));
    return ret;
}
///this function would
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
        if (ysum >0) {ret[i]=1;} else {ret[i]=-1;}
    }
    return ret;
}
Compute_float* STDP_str(const volatile Compute_float* const S) //use volatile here because the tagged array has volatile and the compiler bitches about it.  Not a real problem
{
    Compute_float* ret = malloc(sizeof(*ret)*grid_size*grid_size);
    for (int i=0;i<grid_size*grid_size;i++)
    {
        Compute_float sum = Zero;
        for (int a=-STDP_RANGE;a<STDP_RANGE;a++)
        {
            for (int b=-STDP_RANGE;b<STDP_RANGE;b++)
            {
                sum += fabs( S[i*STDP_array_size*STDP_array_size+(a+STDP_RANGE)*STDP_array_size + (b+STDP_RANGE)]);
            }
        }
    }
    return ret;
}

tagged_array* STDP_mag(const tagged_array* const in)
{
    return tagged_array_new(STDP_str(in->data),in->size,in->offset,in->subgrid,in->minval,in->maxval);
}
