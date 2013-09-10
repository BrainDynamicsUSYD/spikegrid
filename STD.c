#include "STD.h"
#include "parameters.h"
#include <stdlib.h>
void STD_init()
{
    for(int i=0;i < grid_size*grid_size;i++)
    {
        STD.ftimes[i]=0; //progably not required
        STD.U[i] = Param.STD.U;
        STD.R[i] = 1.0;
    }
}
