/// \file
#include "STDP.h"
#include "mymath.h" //fabsf
#define STDP_RANGE_SQUARED (couplerange*couplerange)
//helper function for STDP.  Calculates distance between two neurons, taking into account wrapping in the network
//interesting idea - in some cases I don't care about this wrapping and could cheat
//inline for speed
inline static int dist(int cur,int prev)
{
    int dx = cur-prev;
    if (dx > (grid_size/2)) {return (dx - grid_size);}
    else if (dx < -(grid_size/2)) {return (dx + grid_size);}
    else {return dx;}
}
//invert the distances
inline static int invertdist(int v) {return ((2*couplerange) - v);}
//this method is incredibly hard to understand.  Many weird things are driven by speed.  But basically, implement STDP.
void ApplySTDP(Compute_float * __restrict__ dmats,const coords* curfire,const coords* prevfire,const Compute_float str,const Compute_float* constm,const STDP_parameters S)
{
    int cindex = 0;
    while(curfire[cindex].x != -1)
    {   
        const int nx = curfire[cindex].x ; //switching these to use ++ slows things down
        const int ny = curfire[cindex].y;
        int pindex = 0;
        if (nx < grid_size - couplerange && nx > couplerange) //no wrapping
        {
            if (prevfire[pindex].x > nx+couplerange) {goto loop_end;} //basically, there are no firing neurons in range, so bail out
        }
        while(prevfire[pindex].x != -1)
        {
            const int px = prevfire[pindex].x ; 
            const int py = prevfire[pindex].y;
            if (px > nx + couplerange && nx > couplerange) {break;} //if we go past the end we can break - but need to check we haven't gone to far
            //calculate distance
            const int cx = dist(nx,px);
            const int cy = dist(ny,py);
            if ((cx*cx+cy*cy) < STDP_RANGE_SQUARED)
            {
                const int cdx = cx + couplerange;
                const int cdy = cy + couplerange;
                const int rx = invertdist(cdx); //moving this into loop has no effect - each is one lea instrucion only - possibly never even calculated
                const int ry = invertdist(cdy);
                if((dmats[(px*grid_size+py)*couple_array_size*couple_array_size + cdx*couple_array_size + cdy]) <  fabs(S.stdp_limit * constm[cdx*couple_array_size+cdy])  )
                {   
                    dmats[(px*grid_size+py)*couple_array_size*couple_array_size + cdx*couple_array_size + cdy] += str; //all the lookup code here is cached
                    dmats[(nx*grid_size+ny)*couple_array_size*couple_array_size + rx*couple_array_size + ry]   -= str;
                }
            }
            pindex++;
        }
        loop_end:
            cindex++;
    }
}


//How STDP works:
//We have an array of (int,int) tuples which store locations which are firing at some time step (firestore / flocstore).  
//However, as we only need to know when nearby points are firing, we also have flocdex - this stores were each x-coord starts
//      as a result, we can skip through large parts of the array preemptively, makeing things significantly faster

//Next idea for STDP speed improvements - The Magnitude check is based on the direction from the previous to the current (increasing) - as a result, the innermost loop frequently calculates the offset - swapping curfire and prevfire

void doSTDP (Compute_float* dmats,const ringbuffer* const fdata , const Compute_float*constm,const STDP_parameters S)
{
    const coords* const curfire = fdata->data[fdata->curidx];
    for(unsigned int offset = 1;offset<fdata->count;offset++)
    {
        Compute_float strn = S.stdp_strength* exp(-((Compute_float)offset)/S.stdp_tau);
        const coords* const fire_with_this_lag = ringbuffer_getoffset(fdata,(int)offset);
        ApplySTDP(dmats,curfire,fire_with_this_lag,strn,constm,S);
    }
}
