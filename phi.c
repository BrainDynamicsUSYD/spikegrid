#include "sizes.h"
#include "mymath.h"
Compute_float* CreatePhiMatrix(void)
{
    Compute_float* PhiMat = calloc(sizeof(Compute_float),grid_size*grid_size);
    for (Neuron_coord x = 0;x<grid_size;x++)
    {
        for (Neuron_coord y = 0;y<grid_size;y++)
        {
            const size_t idx = grid_index((coords){.x=x,.y=y});
            PhiMat[idx] = (Compute_float)idx*2*M_PI/(grid_size*grid_size);
        }
    }
    return PhiMat;
}

