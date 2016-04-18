/// \file
#include <stddef.h> //offsetof
//these first few parameters actually escape into the paramheader file through magic
#define grid_size 200
///Total size of the grid
///Coupling range
#define couplerange 15
#ifndef PARAMETERS  //DO NOT REMOVE
///include guard
#define PARAMETERS  //DO NOT REMOVE
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
static const parameters OneLayerModel = {.couple={0}};
#define potparams .potential =     \
{                                  \
    .type    =                     \
    {                           \
        .type = QIF,            \
        .extra =                \
        {                       \
            .QIF={.Vth=-55}     \
        }                       \
    },                          \
    .Vrt     = -70,             \
    .Vpk    = 30,              \
    .Vlk     = -70,             \
    .Vex     = 0,               \
    .Vin     = -80,             \
    .glk     = 0.05,            \
    .rate = 0,                  \
}

#define recparams .recovery =   \
{                               \
    .Wrt = 20,                   \
    .Wir = -1,                  \
    .Wcv = 0.08,                \
}

///parameters for the inhibitory layer of the double layer model
#define stimparams .Stim =                          \
{                                               \
    .ImagePath  = "input_maps/olive100.png",    \
    .timeperiod=1e6,                            \
    .lag=1e6,                                   \
    .PreconditioningTrials=0,                   \
    .NoUSprob=0,                                \
    .Testing=OFF,                               \
    .Periodic=OFF,                              \
    .I2 = 3.20,                                 \
    .I1 = 0,                                    \
    .I0 = 0.80,                                 \
    .mu = 0.04,                                 \
}

static const parameters DualLayerModelIn =
{
    .couple =
    {
        .Layertype = DUALLAYER,
        .Layer_parameters = 
        {
            .dual = 
            {
                .connectivity = HOMOGENEOUS,   //EXPONENTIAL or HOMOGENEOUS
                .W            = -0.30, 
                .sigma        = 60, 
                .synapse      = {.R=0.5,.D=2.0},
            }
        },
        .tref       = 5,
        .norm_type = GlobalMultiplier,
        .normalization_parameters = {.glob_mult = {.GM=1}},
    },
    potparams,
    recparams,
    stimparams,
    .skip = 2,
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
                .connectivity = EXPONENTIAL,   ///EXPONENTIAL or HOMOGENEOUS
                .W            = 0.23, //0.09 //0.12 //0.14  //0.23
                .sigma        = 12,
                .synapse      = {.R=0.5,.D=2.0},
            }
        },
        .tref       = 5,
        .norm_type = GlobalMultiplier,
        .normalization_parameters = {.glob_mult = {.GM=1}},
    },
    potparams,
    recparams,
    stimparams,
    .skip = 1,
};
///Constant external input to conductances
static const extinput Extinput =
{
    .gE0 = 0,
    .gI0 = 0,
};
///Some global features that can be turned on and off
static const model_features Features = 
{
    .Recovery   = ON,
    .STDP		= OFF, //Question - some of these do actually make more sense as a per-layer feature - just about everything that isn't the timestep - 
    .STD        = OFF,  //if we need any of these features we can make the changes then.
    .Theta      = OFF,
    .Timestep   = 0.05, // Works in like with 0.1 for midpoint. But if gE too small should addition be smaller too???
    .Simlength  = 5e4,
    .ImageStim = ON,
    .job        = {.initcond = RAND_JOB, .Voltage_or_count = 1},
    .Outprefix  = "recovery_trials",  // Make empty to keep in current directory
    .output = {
        { .method=SPIKES,.Output="Spike1" ,.Delay=1}, // Exc. spikes
        { .method=SPIKES,.Output="Spike2" ,.Delay=1}, // Inh. spikes
        { .method=TEXT,.Output="gE",.Delay=20},    // Excitation
        { .method=TEXT,.Output="gI",.Delay=20},    // Inhibition
        { .method=TEXT,.Output="V1",.Delay=20},    // Exc. voltage 
        { .method=TEXT,.Output="V2",.Delay=20},    // Inh. voltage
    }, 
};
///Parameters for conducting a parameter sweep.
static const sweepable Sweep =
{
    // .offset=offsetof(parameters,couple.Layer_parameters.dual.W),
    // .minval = -0.22,
    // .maxval = -0.10,
    // .count = 12,
    // .SweepEx = OFF,
    // .SweepIn = ON,
    //
    // .offset=offsetof(parameters,recovery.Wrt),
    // .minval = 5,
    // .maxval = 30,
    // .count = 5,
    // .SweepEx = ON,
    // .SweepIn = ON,
    //
    // .offset=offsetof(parameters,couple.normalization_parameters.glob_mult.GM),
    // .minval = 1,
    // .maxval = 4,
    // .count = 30,
    // .SweepEx = ON,
    // .SweepIn = ON,
    //
    .offset=offsetof(parameters,couple.normalization_parameters.glob_mult.GM),
    .minval = 1,
    .maxval = 1,
    .count = 50,
    .SweepEx = ON,
    .SweepIn = ON,
};

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#endif //DO NOT REMOVE
