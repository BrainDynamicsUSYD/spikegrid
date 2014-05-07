#include "STD.h"
#include "helpertypes.h"
//Initialise the STD parameters to their initial values.  Failing to call this before using STD will give incorrect results initially.
//It is possible that results will converge after some time, but best not to risk it.
STD_data STD_init(const STD_parameters s)
{
    STD_data ret = {.P=(STD_parameters*)newdata(&s,sizeof(s))};
    for(int i=0;i < grid_size*grid_size;i++)
    {
        ret.ftimes[i]=0; //probably not required as you can gurantee that the memory is set to 0 by default
        ret.U[i] = s.U;
        ret.R[i] = One;
    }
    return ret;
}

