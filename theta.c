/// \file
#include "paramheader.h"
#include "mymath.h"
#include "sizes.h"
///Add in theta wave changes to voltages
/// @param vin          input voltages (modeified in place)
/// @param theta        Contains the parameters of the theta wave
/// @param timemillis   Used to calculate the theta strength (as it changes over time)
void dotheta(Compute_float* vin, const theta_parameters theta,const Compute_float timemillis)
{
    for (Neuron_coord x = 0;x<grid_size;x++)
    {
        for (Neuron_coord y = 0;y<grid_size;y++)
        {
            const Compute_float thetastr = theta.strength * sin(timemillis / (Two*((Compute_float)M_PI)*theta.period));
            const size_t idx =grid_index((coords){.x=x,.y=y});
            vin[idx] += thetastr;
        }
    }
}
