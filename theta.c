#include "paramheader.h"
#include "mymath.h"
void dotheta(Compute_float* vin, const theta_parameters theta,const Compute_float timemillis)
{
    for (int x = 0;x<grid_size;x++)
    {
        for (int y = 0;y<grid_size;y++)
        {
            Compute_float thetastr = theta.strength * sin(timemillis / (Two*((Compute_float)M_PI)*theta.period));
            const int idx = (x+couplerange)*conductance_array_size + y + couplerange; //index for gE/gI
            vin[idx] += thetastr;
        }
    }
}
