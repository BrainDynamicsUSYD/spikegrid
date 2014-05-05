#include <stdlib.h> //calloc
#include <stdio.h>  //printf
#include "paramheader.h"
#include "coupling.h" //not actually required at the moment but should ensure that function types match
#include "mymath.h"
/* //This function is useful - but not used
   Compute_float __attribute__((const))exrange(const couple_parameters c)
   {
   return -(c.sigE*c.sigI*logf(c.WE/c.WI))/(c.sigE-c.sigI); //from mathematica
   }
   */
//controls the shape of the synapse.
//TODO: more different types of spikes.
Compute_float __attribute__((pure)) Synapse_timecourse(const decay_parameters Decay,const Compute_float time)
{
    return (One/(Decay.D-Decay.R))*(exp(-time/Decay.D)-exp(-time/Decay.R));
}
//check how far back we need to keep track of histories
unsigned int __attribute__((pure)) setcap(const decay_parameters d,const Compute_float minval, const Compute_float timestep)
{
    Compute_float prev = -1000;//initial values
    Compute_float time=0;
    unsigned int count = 1; //keeps compatibility with matlab
    while(1)
    {
        time+=timestep;
        count++;
        Compute_float alpha=Synapse_timecourse(d,time); 
        // check that the spike is in the decreasing phase and that it has magnitude less than the critical value
        if (alpha<minval && alpha<prev) {break;}
        prev=alpha;
    }
    return count;
}
//normalize the coupling matrix - multiple methods available
Compute_float* Norm_couplematrix(const couple_parameters c, Compute_float* const unnormed)
{
    switch (c.norm_type)
    {
        case None:
            return unnormed;
        case TotalArea:
            {   //Apparently we need a separate scope to declare the variables in - C is weird
                Compute_float plusval = 0;
                Compute_float negval = 0;
                for (int i=0;i<couple_array_size*couple_array_size;i++)
                {
                    const Compute_float val = unnormed[i];
                    if (val < 0) {negval += val;} else {plusval += val;}

                }
                const Compute_float plusnorm = c.normalization_parameters.total_area.WE/plusval;
                const Compute_float negnorm = c.normalization_parameters.total_area.WI/negval;
                for (int i=0;i<couple_array_size*couple_array_size;i++)
                {
                    if (unnormed[i] < 0) {unnormed[i]  *= plusnorm;} else {unnormed[i] *= negnorm;}

                }
                return unnormed;
            }
        case GlobalMultiplier:
            {
                for (int i=0;i<couple_array_size*couple_array_size;i++)
                {
                    unnormed[i] *= c.normalization_parameters.glob_mult.GM;
                }
                return unnormed;
            }
        default:
            printf("unknown normalization method\n");
            return NULL;
    }
}


//compute the mexican hat function used for coupling - should really be marked forceinline or whatever the notation is for GCC.
Compute_float __attribute__((const)) mexhat(const Compute_float rsq,const couple_parameters c){return c.WE*exp(-rsq/c.sigE)-c.WI*exp(-rsq/c.sigI);}

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
                Compute_float val = mexhat((Compute_float)(x*x+y*y),c);//compute the mexican hat function
                if (val>0) {val=val*c.SE;} else {val=val*c.SI;}//and multiply by some constants
                matrix[(x+couplerange)*couple_array_size + y + couplerange] = val;//and set the array
            }
        }
    }
    return Norm_couplematrix(c, matrix);
}

