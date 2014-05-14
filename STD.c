#include "STD.h"
#include <stdlib.h>
///Initialise the STD parameters to their initial values.  
///Failing to call this before using STD will give incorrect results initially.
///It is possible that results will converge after some time, but best not to risk it.
/// @param s the STD parameters
STD_data __attribute__((const)) STD_init(const STD_parameters s)
{
    STD_data ret ;
    ret.U=malloc(sizeof(Compute_float)*grid_size*grid_size);
    ret.R=malloc(sizeof(Compute_float)*grid_size*grid_size);
    for(int i=0;i < grid_size*grid_size;i++)
    {
        ret.ftimes[i]=0; //probably not required as you can gurantee that the memory is set to 0 by default
        ret.U[i] = s.U;
        ret.R[i] = One;
    }
    return ret;
}

