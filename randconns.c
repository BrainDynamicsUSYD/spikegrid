#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "sizes.h"
#include "randconns.h"
#include "paramheader.h"
#include "coupling.h"
//creat a rather ridiculously sized matrix
//allows for 10x the avg number of connections per point.  Incredibly wasteful.  It would be really nice to have some c++ vectors here
const unsigned int overkill_factor = 10;
randconns_info* init_randconns(const randconn_parameters rparam,const couple_parameters couple)
{
    randconns_info rcinfo = {.numberper = rparam.numberper};
    rcinfo.randconns= calloc(sizeof(randomconnection),(size_t)(grid_size*grid_size*rparam.numberper));
    //bigmat will store the points coming towards somewhere (but we don't know precisely how many there will be, so use overkill factor)
    randomconnection** bigmat = calloc(sizeof(randomconnection*),grid_size*grid_size*rparam.numberper*overkill_factor);
    //store the actual counts of connections to a point in bigmatcounts
    unsigned int* bigmatcounts = calloc(sizeof(unsigned int),grid_size*grid_size);
    int nonzcount; //number of non-zero entries in coupling matrix (used for creating coupling strengths)
    Compute_float* interestingconns; //a pointer to use for storing the non-zero entries
    Non_zerocouplings(couple,&interestingconns,&nonzcount);//get non-zero couplings - note these are unnormalized
    srandom((unsigned)0); //seed RNG - WHY???? should probably remove this!!!
    //RCS need some connection strength scaling.  We abuse the globalmultiplier normalization most of the time, when coming from a single point boost them unfairly
    const Compute_float Strmod = Features.FixedRCStart==ON?2.0: One - couple.normalization_parameters.glob_mult.GM;
    for (unsigned int x=0;x<grid_size;x++)
    {
        for (unsigned int y=0;y<grid_size;y++)
        {
            if (Features.FixedRCStart == OFF || (x==0 && y==0))
            {
                for (unsigned int i=0;i<rparam.numberper;i++)
                {
                    const randomconnection rc =
                    {
                        .strength = interestingconns[random()%nonzcount] * Strmod, //random non-zero strength
                        .stdp_strength = Zero,
                        .destination =
                        {   //pick a random destination
                            .x = (Neuron_coord)(((Compute_float)(random()) / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                            .y = (Neuron_coord)(((Compute_float)(random()) / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                        },
                        .source = {.x=x,.y=y},
                    };
                    rcinfo.randconns[(x*grid_size+y)*rparam.numberper + i] = rc; //and store it (forward direction)
                    //the normal matrix stores by where they come from.  Also need to store where they got to.
                    const int tocoordidx = rc.destination.x*grid_size+rc.destination.y;
                    bigmat[tocoordidx*(int)rparam.numberper*(int)overkill_factor +(int)bigmatcounts[tocoordidx]]=&rcinfo.randconns[(x*grid_size+y)*rparam.numberper + i];
                    bigmatcounts[tocoordidx]++;
                    if(bigmatcounts[tocoordidx] > overkill_factor*rparam.numberper)
                    {
                        printf("Overkill factor is not large enough - please make it bigger at dx = %i dy = %i\n",rc.destination.x,rc.destination.y);
                        exit(EXIT_FAILURE);
                    }
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
randomconnection* GetRandomConnsLeaving(const int x,const int y,const randconns_info rcinfo, unsigned int* numberconns)
{
    const int randbase=(x*grid_size+y)*(int)rcinfo.numberper;
    if ((x==0 && y==0) || Features.FixedRCStart==OFF)
    {
        *numberconns = rcinfo.numberper;
        return &(rcinfo.randconns[randbase]);
    }
    else
    {
        *numberconns=0;
        return NULL;
    }
}
randomconnection** GetRandomConnsArriving(const int x,const int y,const randconns_info rcinfo, unsigned int* numberconns)
{
    *numberconns              = rcinfo.rev_pp[x*grid_size+y];
    return &(rcinfo.randconns_reverse[(x*grid_size+y)*(int)rcinfo.numberper*(int)overkill_factor]);
}
Compute_float* RandConnsToMat(const randconns_info* const rcinfo)
{
    Compute_float* strmat = calloc(sizeof(Compute_float),grid_size*grid_size);
    for (int x=0;x<grid_size;x++)
    {
        for(int y=0;y<grid_size;y++)
        {
            unsigned int nconns;
            randomconnection* rc = GetRandomConnsLeaving(x,y,*rcinfo,&nconns);
            for (unsigned int i=0;i<nconns;i++)
            {
                strmat[rc[i].destination.x*grid_size+rc[i].destination.y] += rc[i].stdp_strength;
            }
            randomconnection** rcs = GetRandomConnsArriving(x,y,*rcinfo,&nconns);
            for (unsigned int i=0;i<nconns;i++)
            {
                randomconnection rrc = *rcs[i];
                if (rrc.destination.x != x || rrc.destination.y != y) {printf("error in RCarrive expected dest of %i %i got %i %i\n",x,y,rrc.destination.x,rrc.destination.y);}
            //    strmat[rrc.destination.x*grid_size+rrc.destination.y] -= rrc.stdp_strength;
            }
        }
    }
    return strmat;

}
