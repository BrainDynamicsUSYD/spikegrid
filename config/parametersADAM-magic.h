/// trying to recreate figure 4 from Johnson and redish - using a T maze
/// this will try using a t-maze rather than the y-maze from before
#include <stddef.h> //offsetof
//these first few parameters actually escape into the paramheader file through magic
#define grid_size 200
///Total size of the grid
///Coupling range
#define couplerange 25
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

#define potparams .potential =     \
{                                  \
    .type    =                     \
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

#define stimparams .Stim =                          \
    {                                               \
        .ImagePath  = "input_maps/D10_N200.png",    \
        .timeperiod=1e6,                            \
        .lag=1e6,                                   \
        .PreconditioningTrials=0,                   \
        .NoUSprob=0,                                \
        .Testing=OFF,                               \
        .Periodic=OFF,                              \
        .I2 = 0,                                 \
        .I1 = 0,                                    \
        .I0 = 0.80,                                 \
        .mu = 0.80,                                 \
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
                .W          = -0.49, //-0.5
                .sigma      = 100,
                .synapse    = {.R=0.5,.D=2.0},
            }
        },

        .norm_type = GlobalMultiplier,
        .normalization_parameters = {.glob_mult = {.GM=1}},
        .tref       = 5,
    },
    potparams,
    stimparams,
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
                .W          =  0.25,
                .sigma      = 20,
                .synapse    = {.R=0.5,.D=2.0},
            }
        },
        .tref       = 5,
        .norm_type = GlobalMultiplier,
        .normalization_parameters = {.glob_mult = {.GM=1}},
    },
    potparams,
    .skip=-2,
    stimparams,
};
///Some global features that can be turned on and off
static const model_features Features =
{
    .STD        = OFF,
    .STDP       = OFF,
    .Random_connections = OFF,
    .Timestep   = 0.05,
    .Simlength  = 100000,
    .ImageStim  = ON,
    .job        = {.initcond = RAND_JOB, .Voltage_or_count = 1},
    .Disablewrapping = OFF,
    .Outprefix  = "john2_sweepWI",
    .output = {
        { .method=SPIKES,.Output="Spike1" ,.Delay=1}, // Exc. spikes
        { .method=SPIKES,.Output="Spike2" ,.Delay=1}, // Inh. spikes
        { .method=TEXT,.Output="gE",.Delay=20},    // Excitation
        { .method=TEXT,.Output="gI",.Delay=20},    // Inhibition
        { .method=TEXT,.Output="V1",.Delay=20},    // Exc. voltage 
        { .method=TEXT,.Output="V2",.Delay=20},    // Inh. voltage
    },
};
///Constant external input to conductances
static const extinput Extinput =
{
    .gE0 = 0,
    .gI0 = 0,
};
///Parameters for conducting a parameter sweep.
static const sweepable Sweep =
{
    .offset=offsetof(parameters,couple.Layer_parameters.dual.W),
    .minval = -0.55,
    .maxval = -0.45,
    .count = 10,
    .SweepEx = OFF,
    .SweepIn = ON,
};
#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#endif //DO NOT REMOVE
