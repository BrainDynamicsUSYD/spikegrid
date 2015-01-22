/// \file
#include <stddef.h> //offsetof
//these first few parameters actually escape into the paramheader file through magic
#define grid_size 100
///Total size of the grid
///Coupling range
#define couplerange 15
#ifndef PARAMETERS  //DO NOT REMOVE
///include guard
#define PARAMETERS  //DO NOT REMOVE
//disable warnings about float conversion in this file only
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
//the following typedef must be before the include to get the right compute types
///Whether we are using the single or double layer model
static const LayerNumbers ModelType = DUALLAYER;

//Fun note - with the right optimisations GCC actually will pull these constants inline (for example disassemble evolvept_STDP with STDP off)
///Parameters for the single layer model
static const parameters OneLayerModel = {.couple={0}}; //since unused - shortes possible definition that produces no warnings

#define potparams  .potential =     \
    {                               \
        .type    =                  \
        {                           \
            .type = LIF,            \
        },                          \
        .Vrt     = -70,             \
        .Vpk    = -55,              \
        .Vlk     = -70,             \
        .Vex     = 0,               \
        .Vin     = -80,             \
        .glk     = 0.05,            \
        .rate = 0,                  \
    }

#define STDPparams .STDP=   \
    {                       \
        .stdp_limit=0.2,   \
        .stdp_tau=10,       \
        .stdp_strength=0.005, \
        .STDP_on=ON         \
    }
///parameters for the inhibitory layer of the double layer model
static const parameters DualLayerModelIn =
{
    .couple =
    {
        .Layertype = DUALLAYER,
        .Layer_parameters =
        {
            .dual =
            {
                .W          = -0.39, //-0.40 //-0.57 //-0.70 //-1.25,
                .sigma      = 90,
                .synapse    = {.R=0.5,.D=2.0},
            }
        },
        .norm_type = None,
        .tref       = 5,
    },
    potparams,
    STDPparams,
    .skip=2,
};
///parameters for the excitatory layer of the double layer model
static const parameters DualLayerModelEx =
{
    .couple =
    {
        .Layertype = DUALLAYER,
        .Layer_parameters =
        {
            .dual =
            {
                .W          =  0.13,
                .sigma      = 12,
                .synapse    = {.R=0.5,.D=2.0},
            }
        },
        .tref       = 5,
        .norm_type = None,
    },
    STDPparams,
    potparams,
    .random= {.numberper=5000,.str=1000.0, .Specials=2},
    .skip=1,
};
///Some global features that can be turned on and off
static const model_features Features =
{
    .STDP		= ON, //Question - some of these do actually make more sense as a per-layer feature - just about everything that isn't the timestep -
    .STD        = OFF, //               if we need any of these features we can make the changes then.
    .Timestep   = 0.1,
    .Simlength  = 1000000,
    .UseAnimal     = ON,
    .Random_connections=ON,
};
///Constant external input to conductances
static const extinput Extinput =
{
    .gE0 = 0.0,
    .gI0 = 0.05,
};
///Parameters for conducting a parameter sweep.
static const sweepable Sweep =
{
    .offset=offsetof(parameters,couple)+offsetof(couple_parameters,Layer_parameters) +0+ /*offset in the union is always 0*/  + offsetof(duallayer_parameters,W),
    .minval = 0.0,
    .maxval = 1.0,
    .count = 100
};

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#endif //DO NOT REMOVE
