//these first few parameters actually escape into the paramheader file through magic
#define grid_size 100
#define couplerange 15
#ifndef PARAMATERS  //DO NOT REMOVE
#define PARAMATERS  //DO NOT REMOVE
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wconversion"
#else
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-conversion"
#pragma GCC diagnostic ignored "-Wconversion"
#endif
//the following typedef must be before the include to get the right compute types
#include "paramheader.h"

static const LayerNumbers ModelType = DUALLAYER;

////TODO: compiler does not warn on missing elements - fix
//Fun note - with the right optimisations GCC actually will pull these constants inline (for example disassemble evolvept_STDP with STDP off)
static const parameters OneLayerModel = //the fact that this is static is a little messy - in theory gcc will create a copy for each .c file.  However - in reality, this doesn't appear to happen (perhaps GCC realises that it is const so that only one copy is required.  If the const is ever removed, static will cause incredibly weird behaviour.
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
                .WI     = 0.19,                 //Inhib coupling strength
                .sigI   = 42,                   //char. length for In synapses (int / float?)
                .Ex = {.R=0.5,.D=1.5},          //excitatory rise / decay time
                .In = {.R=0.5,.D=2.0},          //inhibitory rise / decay time
            }
        },
        .tref   = 5,
        .norm_type = None,
    },
    .potential = 
    {
        .Vrt     = -70,                  //reset potential
        .Vth     = -55,                  //Threshold potential
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
        .glk     = 0.05,                 //leak current
        .rate = 1,
    },
    .STDP = 
    {
        .stdp_limit     = 0.1,
        .stdp_tau       = 20,
        .stdp_strength  = 0.0004
    }, 
    .STD =
    {
        .U  = 0.5,
        .D  = 0.11,
        .F  = 0.005
    },
    .Movie = 
    {
        .MakeMovie = OFF,
        .Delay = 10,
    },
    .theta = 
    {
        .strength    = 5.0,
        .period     = 0.2,
    },
};
static const parameters DualLayerModelIn =
{
    .couple =
    {
        .Layertype = DUALLAYER,
        .Layer_parameters = 
        {
            .dual = 
            {
                .W          = -0.41,
                .sigma      = 14,
                .synapse    = {.R=0.5,.D=1.5},
            }
        },
        .norm_type = None,
        .tref       = 5,
    },
    .potential = 
    {
        .Vrt     = -70,                  //reset potential
        .Vth     = -55,                  //Threshold potential
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
        .glk     = 0.05,                 //leak current
        .rate = 1,
    },
    .STDP = 
    {
        .stdp_limit     = 0.1,
        .stdp_tau       = 20,
        .stdp_strength  = 0.0004
    }, 
    .STD =
    {
        .U  = 0.5,
        .D  = 0.11,
        .F  = 0.005
    },
    .Movie = 
    {
        .MakeMovie = OFF,
        .Delay = 10,
    },
    .theta = 
    {
        .strength    = 5.0,
        .period     = 0.2,
    },
};
static const parameters DualLayerModelEx =
{
    .couple =
    {
        .Layertype = DUALLAYER,
        .Layer_parameters = 
        {
            .dual = 
            {
                .W          = 0.41,
                .sigma      = 42,
                .synapse    = {.R=0.5,.D=2},
            }
        },
        .tref       = 5,
        .norm_type = None,
    },
    .potential = 
    {
        .Vrt     = -70,                  //reset potential
        .Vth     = -55,                  //Threshold potential
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
        .glk     = 0.05,                 //leak current
        .rate = 1,
    },
    .STDP = 
    {
        .stdp_limit     = 0.1,
        .stdp_tau       = 20,
        .stdp_strength  = 0.0004
    }, 
    .STD =
    {
        .U  = 0.5,
        .D  = 0.11,
        .F  = 0.005
    },
    .Movie = 
    {
        .MakeMovie = OFF,
        .Delay = 10,
    },
    .theta = 
    {
        .strength    = 5.0,
        .period     = 0.2,
    },
};

static const model_features Features = 
{
    .STDP		= OFF, //Question - some of these do actually make more sense as a per-layer feature - just about everything that isn't the timestep - 
    .STD        = ON , //               if we need any of these features we can make the changes then.
    .Output     = OFF,
    .Theta      = OFF,
    .Timestep   = 0.1,
};

static const sweepable Sweep =
{
    .type = Vrt,
    .minval = 1.0,
    .maxval = 2.0,
    .count = 10
};

#ifdef __clang__
#pragma clang diagnostic pop
#else
#pragma GCC diagnostic pop
#endif
#endif //DO NOT REMOVE
