/// \file
#include <stdlib.h> //calloc
#include <stdio.h>  //printf
#include "paramheader.h"
#include "mymath.h"
/* //This function is useful - but not used
   Compute_float __attribute__((const))exrange(const couple_parameters c)
   {
   return -(c.sigE*c.sigI*logf(c.WE/c.WI))/(c.sigE-c.sigI); //from mathematica
   }
   */
/// Calculates the magnitude of a spike a certain number of milliseconds since the spike was emitted.  The spike shapes should be the same as the matlab version
/// @param time time in milliseconds
/// @param Decay the parameters of the spike
Compute_float  __attribute__((pure)) Synapse_timecourse(const decay_parameters Decay,const Compute_float time)
{
    return (One/(Decay.D-Decay.R))*(exp(-time/Decay.D)-exp(-time/Decay.R));
}
///Calculate how long we need to track spike histories.  This is done by seeing how long it takes the spike magnitude to decrease below a critical value
///@param minval The minimum spike size that we care about.  This is not affected by the coupling matrix at all.  As a result, if your coupling strengths are very high, this might need to be reduced
unsigned int __attribute__((pure)) setcap(const decay_parameters d,const Compute_float minval, const Compute_float timestep)
{
    Compute_float prev = -1000;//initial values
    Compute_float time=0;
    unsigned int count = 2; //3. keeps compatibility with matlab
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
///normalize the coupling matrix - multiple methods available
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
        case MultSep:
            {
                for (int i=0;i<couple_array_size*couple_array_size;i++)
                {
                    const Compute_float val = unnormed[i];
                    if (val < 0) {unnormed[i] *= c.normalization_parameters.mult_sep.Infactor;} else {unnormed[i] *= c.normalization_parameters.mult_sep.Exfactor;}

                }
            }
        default:
            printf("unknown normalization method\n");
            return NULL;
    }
}


/// Computes the mexican-hat based coupling function - used in single layer model
Compute_float __attribute__((const)) mexhat  (const Compute_float rsq,const singlelayer_parameters c){return c.WE*exp(-rsq/c.sigE)-c.WI*exp(-rsq/c.sigI);}
/// Computes the exponentially decaying coupling function used in the double layer model
Compute_float __attribute__((const)) expdecay(const Compute_float rsq,const duallayer_parameters d  ){return d.W *exp(-rsq/d.sigma);}

//I tried changing this function so that it precomputes the multiplication by the spike shape.  However - it turns out that this actually made the code slower as it significantly increased the number of cache misses as well as memory bandwidth.
///Creates the coupling matrix for a layer.
Compute_float* CreateCouplingMatrix(const couple_parameters c)
{
    Compute_float* matrix = calloc(sizeof(Compute_float),couple_array_size*couple_array_size); //matrix of coupling values
    for(int x=-couplerange;x<=couplerange;x++)
    {
        for(int y=-couplerange;y<=couplerange;y++)
        {
            if (x*x+y*y<=couplerange*couplerange)//if we are within coupling range
            {
                if (x!= 0 || y!= 0) //remove self-coupling
                {
                    Compute_float val;
                    if (c.Layertype==SINGLELAYER)
                    {
                        val = mexhat((Compute_float)(x*x+y*y),c.Layer_parameters.single);//compute the mexican hat function
                    }
                    else
                    {
                        if (c.Layer_parameters.dual.connectivity==EXPONENTIAL)
                        {
                            val = expdecay((Compute_float)(x*x+y*y),c.Layer_parameters.dual);
                        }
                        else 
                        {
                            val = c.Layer_parameters.dual.W;
                        }
                    }
                    matrix[(x+couplerange)*couple_array_size + y + couplerange] = val;//and set the array
                }
            }
        }
    }
    return Norm_couplematrix(c, matrix);
}
///Cache the shape of the spike
///In the conductance model, neurons emit spikes that decay gradually over time.  This function pre-calculates these strengths
/// @param cap The maximum time to still add a spike @see setcap
/// @param Decay used to calculate spike magnitude over time @see Synapse_timecourse
/// @param timestep Required for various parts of the calculation
Compute_float* __attribute__((const)) Synapse_timecourse_cache (const unsigned int cap, const decay_parameters Decay,const Compute_float timestep)
{
    Compute_float* ret = calloc(sizeof(Compute_float),cap);
    for (unsigned int i=0;i<cap;i++)
    {
        const Compute_float time = ((Compute_float)i)*timestep;
        ret[i]=Synapse_timecourse(Decay,time); 
    }
    return ret;
}
