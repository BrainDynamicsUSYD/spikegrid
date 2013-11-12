#include "parameters.h"
#include "coupling.h" //not actually required at the moment but should ensure that function types match
#include <tgmath.h> //logf / exp
#include <stdlib.h> //calloc

/* //This function is useful - but not used
 Compute_float __attribute__((const))exrange(const couple_parameters c)
{
   return -(c.sigE*c.sigI*logf(c.WE/c.WI))/(c.sigE-c.sigI); //from mathematica
}
*/
//check how far back we need to keep track of histories
int setcap(const Compute_float D,const Compute_float R,const Compute_float minval)
{
    Compute_float prev = -1000;//initial values
    Compute_float alpha = 0;
    Compute_float time=0;
    Compute_float norm=One/(D-R);
    while(1)
    {
        time+=Param.time.dt;
        alpha=(exp(-time/D) - exp(-time/R))*norm;
        // check that the spike is in the decreasing phase and that it has magnitude less than the critical value
        if (alpha<minval && alpha<prev) {break;}
        prev=alpha;
    }
    return (int)(time/Param.time.dt) +1; //this keeps compatibility with the matlab - seems slightly inelegent - maybe remove
}

//compute the mexican hat function used for coupling
Compute_float mexhat(const Compute_float rsq,const couple_parameters c){return c.WE*exp(-rsq/c.sigE)-c.WI*exp(-rsq/c.sigI);}

//does what it says on the tin
Compute_float* CreateCouplingMatrix(const couple_parameters c)
{
    Compute_float* matrix = calloc(sizeof(Compute_float),couple_array_size*couple_array_size); //matrix of coupling values
    for(int x=-couplerange;x<=couplerange;x++)
    {
        for(int y=-couplerange;y<=couplerange;y++)
        {
            if (x*x+y*y<=couplerange*couplerange)//if we are within coupling range
            {
                float val = mexhat((Compute_float)(x*x+y*y),c);//compute the mexican hat function
                if (val>0) {val=val*c.SE;} else {val=val*c.SI;}//and multiply by some constants
                matrix[(x+couplerange)*couple_array_size + y + couplerange] = val;//and set the array
            }
        }
    }
    return matrix;
}

