
#include <stdlib.h>
#include "model.h"
//need a constructor for initing to 0
condmat::condmat()
{
    for (Neuron_coord i=0;i<grid_size;i++)
    {
        for (Neuron_coord j=0;j<grid_size;j++)
        {
            const size_t idx = Conductance_index((coords){.x=i,.y=j});
            this->gI[idx]=0;
            this->gE[idx]=0;
        }
    }
}
condmat::condmat(const extinput inp)
{
    for (Neuron_coord i=0;i<grid_size;i++)
    {
        for (Neuron_coord j=0;j<grid_size;j++)
        {
            const size_t idx = Conductance_index((coords){.x=i,.y=j});
            this->gI[idx]=inp.gI0;
            this->gE[idx]=inp.gE0;
        }
    }
}

model::model(const parameters p,const parameters p2,const model_features F,const int trefrac_in_ts,const LayerNumbers lcount,const extinput inp):
    layer1(p,F,trefrac_in_ts),
    layer2(p2,F,trefrac_in_ts), //TODO - how to avoid this call in single layer - maybe bail in the constructor?
    timesteps(0),
    NoLayers(lcount),
    cond_matrices(),
    cond_matrices_init(inp)
{
    this->animal = (Animal*)calloc(sizeof(animal),1);
}

model * makemodel(const parameters p,const parameters p2,const model_features F,const int trefrac_in_ts,const LayerNumbers lcount,const extinput inp)
{
    return new model(p,p2,F,trefrac_in_ts,lcount,inp);
}
