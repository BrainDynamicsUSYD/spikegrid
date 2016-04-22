/// \file
#include <stdlib.h>
#include <string.h>
#include "lagstorage.h"

///The lagstorage functions have some issues - mainly while loops to find the end etc.  However, it looks like it is
///slower to keep track of the count
///One thing to fix would be auto-calculating lengths - bit it is a little tricky as STDP uses a different mechanism
lagstorage* lagstorage_init(const unsigned int flagcount,const int cap)
{
    lagstorage firinglags =
    {
        .lags         = calloc(sizeof(int16_t),grid_size*grid_size*(size_t)flagcount), //TODO: switching to store a timestep number here might be faster
        .counts         = calloc(sizeof(int16_t),grid_size*grid_size),
        .cap          = cap,
        .lagsperpoint = flagcount
    };
    for (Neuron_coord x = 0;x<grid_size;x++)
    { //initialize firing lags - essentially sets up an initial condition with no spikes in the past.  If you wanted spikes before the start of the simulation - change this
        for (Neuron_coord y = 0;y<grid_size;y++)
        {
            const coords c = {.x=x,.y=y};
            firinglags.lags[LagIdx(c,&firinglags)]= -1;
        }
    }
    lagstorage* l = malloc(sizeof(*l)); //otherwise we are returning a stack variable
    memcpy(l,&firinglags,sizeof(*l));
    return l;
}

void lagstorage_dtor(lagstorage* l)
{
    free(l->lags);
    free(l); //lagstorages are always allocated with malloc
}
#include<stdio.h>
int16_t __attribute__((const,pure)) CurrentShortestLag(
        const lagstorage* const L,
        const size_t  baseidx,
        const size_t realbase)
{
    const uint16_t count = L->counts[realbase];
    if (count==0) {return INT16_MAX;} else {return L->lags[baseidx+count-1];}
}
void AddnewSpike(lagstorage* L,const size_t baseidx)
{
    const size_t realbase = baseidx/L->lagsperpoint;
    //find the empty idx
    const size_t idx = (size_t)L->counts[realbase];
    //and set it to 1 - this makes things work
    L->lags[baseidx + idx]=1;
    //and set the next one to -1 to mark the end of the array
    L->lags[baseidx + idx+1]= -1;
    L->counts[realbase]++;
}
//called for every neuron on every timestep
void RemoveDeadSpike(lagstorage* L,const size_t baseidx)
{
    const size_t realbase = baseidx/L->lagsperpoint;
    L->counts[realbase]--;
    unsigned int idx2 = 0;
    while (L->lags[baseidx+idx2] != -1) //move everthing down
    {
        L->lags[baseidx+idx2] = L->lags[baseidx+idx2+1]; //since this is the next one, we will always move the -1 as well
        idx2++;
    }
}
//be careful - this function uses a pretty significant amount of time - called for every neuron at every timestep (twice with STDP)
//
void modifyLags(lagstorage* L,const size_t baseidx,const size_t realbase)
{
    if (L->lags[baseidx]==L->cap-1) {RemoveDeadSpike(L,baseidx);}
    const uint16_t count =L->counts[realbase] ;
    //increment the firing lags.
    for (uint16_t idx=0;idx<count;idx++)
    {
        L->lags[baseidx+idx]++;
        //maybe we could use some trick SSE instruction.
        //otherwise - maybe store a timestep number
    }
 //   RemoveDeadSpike(L,baseidx); //the structure here could be nicer - however I wouldn't be surprised if gcc does some magic and auto inlines and reorders removedeadspike and modifylags
}

