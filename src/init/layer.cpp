#include <string.h>
#include "layer.h"
extern "C"
{
    #include "../phi.h"
    #include "../randconns.h"
    #include "../coupling.h"
    #include "../STDP.h"
}
Phimat::Phimat(const parameters p, const model_features f)
{
    if (p.Stim.Periodic==OFF && f.ImageStim==ON)
    {
        memcpy((Compute_float*)this->phimat,CreatePhiMatrix(),grid_size*grid_size*sizeof(Compute_float));
    }
}
Conmat::Conmat(const couple_parameters c)
{
    memcpy((Compute_float*)this->connections,CreateCouplingMatrix(c),sizeof(Compute_float)*couple_array_size*couple_array_size); 
}
RD_data::RD_data(const decay_parameters in):
    R(in.R),
    D(in.D)
{
}
layer::layer(const parameters p, const model_features f,const int trefrac_in_ts):
    phimat(p,f),
    // Phimat(static_cast<Compute_float[grid_size*grid_size]>(CreatePhiMatrix())),
    connections(p.couple),
    //   connections(CreateCouplingMatrix(p.couple)),
    //voltages(0),
    //  recoverys(0),
    rcinfo(f.Random_connections==ON?init_randconns(p.random,p.couple): NULL),
    //lags here - what?
    STDP_Data(f.STDP==ON?STDP_init(&p.STDP,trefrac_in_ts):NULL),
    P(p),
    //std data here
    Layer_is_inhibitory((p.couple.Layertype==DUALLAYER && p.couple.Layer_parameters.dual.W<0)?on_off::ON:on_off::OFF),
    RD(p.couple.Layer_parameters.dual.synapse)
    //RD here - actually doable in cpp
{
    for (int i=0;i<grid_size*grid_size;i++)
    {
        this->lags.lags[i]=0;
    }
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            coords c;
            c.x=i;
            c.y=j;
            const size_t idx = grid_index(c);
            this->voltages.Out[idx]=-100;
        }
    }
    this->lags.trefrac_in_ts = (uint8_t)trefrac_in_ts;
}
layer* makelayer(const parameters p, const model_features f, const int trefrac_in_ts)
{
    return new layer(p,f,trefrac_in_ts);
}
