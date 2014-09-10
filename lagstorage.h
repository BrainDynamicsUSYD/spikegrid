/// \file
#ifndef LAGSTORAGE
#define LAGSTORAGE
#include <stdint.h>
typedef struct lagstorage
{
    int16_t*    lags;
    const int   cap;
    const int   lagsperpoint;
} lagstorage;

void AddnewSpike(lagstorage* L,const int baseidx);
void modifyLags(lagstorage* L,int baseidx);
int16_t __attribute__((const,pure)) CurrentShortestLag(const lagstorage* const L,const int baseidx);
#endif
