#include <stdlib.h>
#include <stdio.h>
#include "sizes.h"
#include "randconns.h"
#include "paramheader.h"
#include "coupling.h"
randconns_info init_randconns(const randconn_parameters rparam,const couple_parameters couple)
{
    randconns_info rcinfo;
    rcinfo.randconns= calloc(sizeof(randomconnection),(size_t)(grid_size*grid_size*rparam.numberper));
    //creat a rather ridiculously sized matrix
    //allows for 10x the avg number of connections per point.  Incredibly wasteful.  It would be really nice to have some c++ vectors here
    const unsigned int overkill_factor = 10;
    randomconnection** bigmat = calloc(sizeof(randomconnection*),grid_size*grid_size*rparam.numberper*overkill_factor);
    unsigned int* bigmatcounts = calloc(sizeof(unsigned int),grid_size*grid_size);
    int nonzcount;
    Compute_float* interestingconns;
    Non_zerocouplings(couple,&interestingconns,&nonzcount);
    srandom((unsigned)0);
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
                        .strength = interestingconns[random()%nonzcount] * Strmod,
                        .stdp_strength = Zero,
                        .destination =
                        {
                            .x = (Neuron_coord)(((Compute_float)(random()) / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                            .y = (Neuron_coord)(((Compute_float)(random()) / (Compute_float)RAND_MAX) * (Compute_float)grid_size),
                        }
                    };
                    rcinfo.randconns[(x*grid_size+y)*rparam.numberper + i] = rc;
                    //the normal matrix stores by where they come from.  Also need to store where they got to.
                    bigmat[(rc.destination.x*grid_size+rc.destination.y)*(int)rparam.numberper*(int)overkill_factor + (int)bigmatcounts[rc.destination.x*grid_size+rc.destination.y]]=&rcinfo.randconns[(x*grid_size+y)*rparam.numberper + i];
                    bigmatcounts[rc.destination.x*grid_size+rc.destination.y]++;
                    if(bigmatcounts[rc.destination.x*grid_size+rc.destination.y] > overkill_factor*rparam.numberper)
                    {
                        printf("Overkill factor is not large enough - please make it bigger at dx = %i dy = %i\n",rc.destination.x,rc.destination.y);
                        exit(EXIT_FAILURE);
                    }
                }
            }
        }
    }
    randomconnection** rev_conns = malloc(sizeof(randomconnection*)*grid_size*grid_size*rparam.numberper);
    randomconnection*** rev_conns_lookup = malloc(sizeof(randomconnection**)*grid_size*grid_size);
    unsigned int* rev_pp = malloc(sizeof(unsigned int)*grid_size*grid_size);
    int count = 0;

    for (unsigned int x=0;x<grid_size;x++)
    {
        for (unsigned int y=0;y<grid_size;y++)
        {
            rev_conns_lookup[x*grid_size+y] = &rev_conns[count];
            unsigned int mycount = 0;
            while(bigmat[(x*grid_size+y)*rparam.numberper*overkill_factor + mycount] != NULL && mycount < overkill_factor)
            {
                rev_conns[count]=bigmat[(x*grid_size+y)*rparam.numberper*overkill_factor + mycount];
                count++;mycount++;
            }
            rev_pp[x*grid_size+y]=mycount;
        }
    }
    rcinfo.rev_pp=rev_pp;
    rcinfo.randconns_reverse=rev_conns;
    rcinfo.randconns_reverse_lookup = rev_conns_lookup;
    free(bigmat);
    free(bigmatcounts);
    return rcinfo;
}
randomconnection* GetRandomConnsLeaving(const int x,const int y,const randconns_info rcinfo,const randconn_parameters* const rparam, unsigned int* numberconns)
{
    const int randbase=(x*grid_size+y)*(int)rparam->numberper;
    if ((x==0 && y==0) || Features.FixedRCStart==OFF)
    {
        *numberconns = rparam->numberper;
        return &(rcinfo.randconns[randbase]);
    }
    else
    {
        *numberconns=0;
        return NULL;
    }
}
