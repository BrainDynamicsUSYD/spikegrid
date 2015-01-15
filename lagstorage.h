/// \file
#ifndef LAGSTORAGE
#define LAGSTORAGE
#include <stdint.h>
#include "sizes.h"
typedef struct lagstorage
{
    int16_t*    lags;
    const int   cap;
    const int   lagsperpoint;
} lagstorage;

lagstorage lagstorage_init(const int flagcount,const int cap);
void AddnewSpike(lagstorage* L,const int baseidx);
void modifyLags(lagstorage* L,int baseidx);
int16_t __attribute__((const,pure)) CurrentShortestLag(const lagstorage* const L,const int baseidx);
///A small helper function to calculate the base idx for the lags at a given x and y coordinate.  Mostly just for convenience
inline static int __attribute__((const,pure)) LagIdx(const int x,const int y,const lagstorage L) {return (x*grid_size+y)*L.lagsperpoint;}
#endif
