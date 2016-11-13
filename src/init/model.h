///Allows for having multiple layers and simulating them
///Thitypedef struct animal animal;
#ifndef MODELCPP
#define MODELCPP
#ifdef __cplusplus
extern "C" {
    #endif
    #include "../animal.h"
    #ifdef __cplusplus
}
#endif
#include "layer.h"
struct condmat {
#ifdef __cplusplus
    condmat();
    condmat(const extinput inp);
#endif
    Compute_float gE [conductance_array_size*conductance_array_size];
    Compute_float gI [conductance_array_size*conductance_array_size];
};
typedef struct condmat condmat;
///This holds both layes and things that we need to keep track of over time that aren't in the layers (main example gE,gI)
typedef struct model
{
#ifdef __cplusplus
    model(const parameters p,const parameters p2,const model_features F,const int trefrac_in_ts,const LayerNumbers lcount,const extinput inp);
#endif
    layer layer1;                                                       ///< First layer
    layer layer2;
    unsigned int timesteps;
    const LayerNumbers NoLayers;                                        ///<Whether this is a single or double layer model
    Animal*         animal;
    ///Make these part of the struct to ensure they are nearby in memory - however it means you can't allocate a model on the stack
    condmat  cond_matrices;
    const condmat cond_matrices_init;

} model;
#ifdef __cplusplus
extern "C"
{
    #endif
model * makemodel(const parameters p,const parameters p2,const model_features F,const int trefrac_in_ts,const LayerNumbers lcount,const extinput inp);
    #ifdef __cplusplus
}
#endif
#endif
