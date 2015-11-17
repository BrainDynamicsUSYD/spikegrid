#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "randconns.h"
#include "paramheader.h"
#include "coupling.h"
#include "mymath.h"
#include "sizes.h"
//creat a rather ridiculously sized matrix
//allows for 10x the avg number of connections per point.  Incredibly wasteful.  It would be really nice to have some c++ vectors here
const unsigned int overkill_factor = 20;

randconns_info init_randconns_info(const randconn_parameters p)
{
    return (randconns_info) {
        .numberper          = p.numberper,
        .nospecials         = p.Specials,
        .UsingFancySpecials = p.FancySpecials,
        .SpecialAInd        = p.SpecialAInd,
        .SpecialBInd        = p.SpecialBInd,
    };
}

void StoreNewRandconn(const randconns_info* rcinfo,const randomconnection rc,const unsigned int sourceidx,randomconnection** bigmat,unsigned int* const bigmatcounts)
{
    const size_t sourcegrididx=grid_index(rc.source);
    randomconnection* rcp; //use this as a reference once we have put the connection in the storage location for the forward direction
    //store forward randconn
    if (rcinfo->UsingFancySpecials==ON)
    {
        if (sourcegrididx==rcinfo->SpecialAInd)
        {
            rcinfo->Aconns[sourceidx]=rc;
            rcp = &rcinfo->Aconns[sourceidx];
        }
        else if (sourcegrididx==rcinfo->SpecialBInd)
        {
            rcinfo->Bconns[sourceidx]=rc;
            rcp = &rcinfo->Bconns[sourceidx];
        }
        else
        {
            printf("Massive error in init_randconns - tried to create a randconn when using fancy specials that didn't go from a special index\n");
            exit(EXIT_FAILURE);
        }
    }
    else
    { //the easy case
        const size_t myidx=sourcegrididx*rcinfo->numberper+sourceidx;
        rcinfo->randconns[myidx]=rc;
        rcp = &rcinfo->randconns[myidx];
    }
    //store the reverse one
    const size_t destgrididx=grid_index(rc.destination);
    if (rcinfo->UsingFancySpecials==ON)
    {
        //do something tricky here
        //The trick we use is that we assume that the number of connections/neuron << overkill factor
        //This should be OK as there is no reason for more than 1 connection / neuron
        //current parameters work out at 0.2 connections/neuron so should be fine
        //TODO: later on this will need to be taken in to account
        bigmat[destgrididx*overkill_factor +bigmatcounts[destgrididx]]= rcp;
        bigmatcounts[destgrididx]++;
        if(bigmatcounts[destgrididx] > overkill_factor)
        {
            printf("Overkill factor is not large enough - please make it bigger at dx = %i dy = %i\n",rc.destination.x,rc.destination.y);
            exit(EXIT_FAILURE);
        }
    }
    else
    {
        bigmat[destgrididx*rcinfo->numberper*overkill_factor +bigmatcounts[destgrididx]]=rcp;
        bigmatcounts[destgrididx]++;
        if(bigmatcounts[destgrididx] > overkill_factor*rcinfo->numberper)
        {
            printf("Overkill factor is not large enough - please make it bigger at dx = %i dy = %i\n",rc.destination.x,rc.destination.y);
            exit(EXIT_FAILURE);
        }
    }

}

randconns_info* init_randconns(const randconn_parameters rparam,const couple_parameters couple)
{
    randconns_info rcinfo = init_randconns_info(rparam);
    //I will need to tweak this structure for the clever specials - otherwise we use too much ram - even the first one here fails
    if (rcinfo.UsingFancySpecials==OFF)
    {
        rcinfo.randconns= calloc(sizeof(randomconnection),(size_t)(grid_size*grid_size*rparam.numberper));
    }
    else
    {
        rcinfo.Aconns= calloc(sizeof(randomconnection),(size_t)rparam.numberper);
        rcinfo.Bconns= calloc(sizeof(randomconnection),(size_t)rparam.numberper);
    }
    //bigmat will store the points coming towards somewhere (but we don't know precisely how many there will be, so use overkill factor)
    randomconnection** bigmat = calloc(sizeof(randomconnection*),
            (rcinfo.UsingFancySpecials==ON?1:rparam.numberper) *
            grid_size*grid_size*overkill_factor );
    //store the actual counts of connections to a point in bigmatcounts
    unsigned int* bigmatcounts = calloc(sizeof(unsigned int),grid_size*grid_size);
    int nonzcount; //number of non-zero entries in coupling matrix (used for creating coupling strengths)
    Compute_float* interestingconns; //a pointer to use for storing the non-zero entries
    Non_zerocouplings(couple,&interestingconns,&nonzcount);//get non-zero couplings - note these are unnormalized
    //RCS need some connection strength scaling.  We abuse the globalmultiplier normalization most of the time, when coming from a single point boost them unfairly
    const Compute_float Strmod = (rparam.Specials>0 || rcinfo.UsingFancySpecials==ON )?1: One - couple.normalization_parameters.glob_mult.GM;
    for (Neuron_coord x=0;x<grid_size;x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {
            const coords c = {.x=x,.y=y};
            //the specials is if we only have a limited number of neurons with random connections
            //this conditional is crazy and stupid - TODO fixme
            if (grid_index(c) < rcinfo.nospecials || (rcinfo.nospecials==0 && !rcinfo.UsingFancySpecials) || (rcinfo.UsingFancySpecials && (grid_index(c)==rcinfo.SpecialAInd || grid_index(c)==rcinfo.SpecialBInd) ) ) //massive hacky conditional
            {
                printf("makeing connections at %i Aind %i Bind %i\n",x*grid_size+y,rcinfo.SpecialAInd,rcinfo.SpecialBInd);
                for (unsigned int i=0;i<rparam.numberper;i++)
                {
                    const randomconnection rc =
                    {
                        .strength = interestingconns[random()%nonzcount] * Strmod, //random non-zero strength
                        .stdp_strength = Zero,
                        .destination =
                        {   //pick a random destination
                            .x = (Neuron_coord)(RandFloat() * (Compute_float)grid_size),
                            .y = (Neuron_coord)(RandFloat() * (Compute_float)grid_size),
                        },
                        .source = c,
                    };
                    StoreNewRandconn(&rcinfo,rc,i,bigmat,bigmatcounts);
                }
            }
        }
    }
    rcinfo.rev_pp=bigmatcounts;
    rcinfo.randconns_reverse=bigmat;
    randconns_info* rcpoint = malloc(sizeof(*rcpoint));
    memcpy(rcpoint,&rcinfo,sizeof(*rcpoint));
    return rcpoint;
}
randomconnection* GetRandomConnsLeaving(const coords coord ,const randconns_info rcinfo, unsigned int* numberconns)
{
    const size_t idx = grid_index(coord);
    if (rcinfo.UsingFancySpecials==ON) //first case - fancy specials
    {
        if (idx==rcinfo.SpecialAInd)
        {
            *numberconns=rcinfo.numberper;
            return rcinfo.Aconns;
        }
        else if (idx==rcinfo.SpecialBInd)
        {
            *numberconns=rcinfo.numberper;
            return rcinfo.Bconns;
        }
        else
        {
            *numberconns=0;
            return NULL;
        }
    }
    else if (idx < rcinfo.nospecials || rcinfo.nospecials==0 ) //normal case - normalspecials==0 or we have one of the specials
    {
        const size_t randbase=idx*rcinfo.numberper;
        *numberconns = rcinfo.numberper;
        return &(rcinfo.randconns[randbase]);
    }
    else //should only happen when normal specials are used and we have a large input index
    {
        *numberconns=0;
        return NULL;
    }
}
randomconnection** GetRandomConnsArriving(const coords coord,const randconns_info rcinfo, unsigned int* numberconns)
{
    const size_t idx = grid_index(coord);
    //the number is always fine to get
    *numberconns = rcinfo.rev_pp[idx];
    if (rcinfo.UsingFancySpecials==ON)
    {   //the fancy specials store less stuff in the reverse array, so we can just use it with a modified index
        return &(rcinfo.randconns_reverse[idx*(size_t)overkill_factor]); //Ensure you return the address of this object - it is definitely required
    }
    else
    {
        return &(rcinfo.randconns_reverse[idx*(size_t)rcinfo.numberper*(size_t)overkill_factor]); //Ensure you return the address of this object - it is definitely required
    }
}
