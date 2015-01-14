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
static const parameters OneLayerModel = 
{
    .couple =
    {
        .Layertype = SINGLELAYER,
        .Layer_parameters = 
        {
            .single = 
            {
                .WE     = 0.41,                 //excitatory coupling strength
                .sigE   = 14,                   //char. length for Ex symapses (int / float?)
                .WI     = 0.24,                 //Inhib coupling strength
                .sigI   = 42,                   //char. length for In synapses (int / float?)
                .Ex = {.R=0.5,.D=2.0},          //excitatory rise / decay time
                .In = {.R=0.5,.D=2.0},          //inhibitory rise / decay time
            }
        },
        .tref   = 5,
        .norm_type = None,
    },
    .potential = 
    {
        .type    = 
        {
            .type = QIF,
            .extra = 
            {
                .EIF={.Vth=-55,.Dpk=1}
            }
        },
        .Vrt     = -70,                  //reset potential
        .Vpk    = 30,                   //peak potential (at which membrane potential is reset -- must match Vth for LIF neurons)
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
        .glk     = 0.05,                 //leak current
        .rate = 0,
    },
    .recovery = 
    {
        .Wrt = 2,
        .Wir = -1,
        .Wcv = 0.08
    },
    .skip=1,
};
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
                .connectivity = HOMOGENEOUS,   //EXPONENTIAL or HOMOGENEOUS
                .W            = -0.30, 
                .sigma        = 60, 
                .synapse      = {.R=0.5,.D=7.0},
            }
        },
        .norm_type = None,
        .tref       = 5,
    },
    .potential = 
    {
        .type    = 
        {
            .type = QIF,
            .extra = 
            {
                .QIF={.Vth=-55}
            }
        },
        .Vrt     = -70,                  //reset potential
        .Vpk    = 30,                   //peak potential (at which membrane potential is reset -- must match Vth for LIF neurons)
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
        .glk     = 0.05,                 //leak current
        .rate = 0,
    },    
    .recovery = 
    {
        .Wrt = 2,
        .Wir = -1,
        .Wcv = 0.08,
    },
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
        .norm_type = None,
    },
    .potential = 
    {
        .type    = 
        {
            .type = QIF,
            .extra = 
            {
                .QIF={.Vth=-55}
            }
        },
        .Vrt     = -70,                  //reset potential
        .Vpk    = 30,                   //peak potential (at which membrane potential is reset -- must match Vth for LIF neurons)
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
        .glk     = 0.05,                 //leak current
        .rate = 0,
    },
    .recovery = 
    {
        .Wrt = 2,
        .Wir = -1,
        .Wcv = 0.08,
    },
    .skip = 1,
};
///Constant external input to conductances
static const extinput Extinput =
{
    .gE0 = 0.015,
    .gI0 = 0.002,
};
///Some global features that can be turned on and off
static const model_features Features = 
{
    .Recovery   = ON,
    .STDP		= OFF, //Question - some of these do actually make more sense as a per-layer feature - just about everything that isn't the timestep - 
    .STD        = OFF,  //if we need any of these features we can make the changes then.
    .Theta      = OFF,
    .Timestep   = 0.05, // Works in like with 0.1 for midpoint. But if gE too small should addition be smaller too???
    .Simlength  = 1e4,
    .Outprefix  = "testout",  // Make empty to keep in current directory
    .output = {{ .method=TEXT,.Output=0,.Delay=20}, { .method=TEXT,.Output=1,.Delay=20}, { .method=TEXT,.Output=5,.Delay=20}, { .method=TEXT,.Output=7,.Delay=20}, { .method=SPIKES, .Output=15, .Delay=1}}, 
    // .job=
    // { 
    //     .initcond=RAND_JOB,  //random - run a few times to be sure
    //     .Voltage_or_count = 1,
    // },
};
///Parameters for conducting a parameter sweep.
static const sweepable Sweep =
{
    //.offset=offsetof(parameters,couple)+offsetof(couple_parameters,Layer_parameters)+offsetof(duallayer_parameters,W),
    .offset=offsetof(parameters,recovery)+offsetof(recovery_parameters,Wcv),
    .minval = 0,
    .maxval = 0.5,
    .count = 25,
    .SweepEx = ON,
    .SweepIn = ON,
};

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#endif //DO NOT REMOVE
