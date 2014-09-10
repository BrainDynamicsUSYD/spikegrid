/// \file
#include "lagstorage.h"
int16_t __attribute__((const,pure)) CurrentShortestLag(const lagstorage* const L,const int baseidx)
{
    int idx=0;
    while (L->lags[baseidx + idx] != -1)
    {
        idx++;
    } //take the last entry - then check if in refrac period and set voltages
    if (idx != 0)
    {
        return L->lags[baseidx+idx - 1]; //-1 or otherwise we return -1
    }
    else {return INT16_MAX;}

}
void AddnewSpike(lagstorage* L,const int baseidx)
{
    //find the empty idx
    int idx = 0;
    while (L->lags[baseidx + idx] != -1)
    {
        idx++;
    }
    //and set it to 1 - this makes things work
    L->lags[baseidx + idx]=1;
    //and set the next one to -1 to mark the end of the array
    L->lags[baseidx + idx+1]= -1;
}
void RemoveDeadSpike(lagstorage* L,const int baseidx)
{
    if (L->lags[baseidx] == L->cap )//if first entry is at cap - remove and shuffle everything down
    {
        int idx2 = 0;
        while (L->lags[baseidx+idx2] != -1) //move everthing down
        {
            L->lags[baseidx+idx2] = L->lags[baseidx+idx2+1]; //since this is the next one, we will always move the -1 as well
            idx2++;
        }
    }
}
void modifyLags(lagstorage* L,int baseidx)
{
    //increment the firing lags.
    int idx = 0;
    while (L->lags[baseidx+idx] != -1)
    {
        L->lags[baseidx+idx]++;
        idx++;
    }
    RemoveDeadSpike(L,baseidx);
}

