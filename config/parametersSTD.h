//Simple file for testing with STD
///Total size of the grid
#define grid_size 100
///Coupling range
#define couplerange 15
#ifndef PARAMETERS  //DO NOT REMOVE
///include guard
#define PARAMETERS  //DO NOT REMOVE
///Whether we are using the single or double layer model
static const LayerNumbers ModelType = DUALLAYER;


///Parameters for the single layer model - we are not using this
static const parameters OneLayerModel = {0}; //since unused - shortest possible definition that produces no warnings

//first define some parameters that are common across both layers
//these define some parameters of a single neuron - things like threshold voltage etc.
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

//parameters for the STD.  Parameter names are as per the Tsodkys papers
#define STDparams .STD =   \
{                           \
    .U= 0.5,\
    .D=0.2,\
    .F=0.45,\
}

///parameters for the inhibitory layer of the double layer model
static const parameters DualLayerModelIn =
{
    .couple =
    {
        .Layertype = DUALLAYER,
        //define how neurons are coupled as well as the spike shape
        .Layer_parameters =
        {
            .dual =
            {
                .W          = -0.79, //-0.40 //-0.57 //-0.70 //-1.25,
                .sigma      = 90,
                .synapse    = {.R=0.5,.D=7.0},
            }
        },
        .norm_type = None,
        .tref       = 5,
    },
    potparams,
    STDparams,
    .skip=2,
};
///parameters for the excitatory layer of the double layer model - mostly similar to the excitatory layer
static const parameters DualLayerModelEx =
{
    .couple =
    {
        .Layertype = DUALLAYER,
        .Layer_parameters =
        {
            .dual =
            {
                .W          =  0.24,
                .sigma      = 12,
                .synapse    = {.R=0.5,.D=2.0},
            }
        },
        .tref       = 5,
        .norm_type = None,
    },
    potparams,
    STDparams,
    .skip=1,
};
///Some global features that can be turned on and off
static const model_features Features =
{
    .STD        = ON,
    .Timestep   = 0.1,
    .Simlength  = 50000,
    //What will be output.  For various reasons, layer "2" is excitatory, which has output that is easier to understand
    .output = {{.method = VIDEO,.Output="V2",.Delay=20, .Overlay="Trialno"},{.method=GUI,.Output="V2",.Delay=10,.Overlay="Timestep"}, {.method=GUI,.Output="STDU2",.Delay=10},{.method=GUI,.Output="STDR2",.Delay=10}}
};
///Constant external input to conductances - used to drive some initial spiking
static const extinput Extinput =
{
    .gE0 = 0.015,
    .gI0 = 0.0,
};
///Parameters for conducting a parameter sweep. = unused
static const sweepable Sweep =
{
    0
};
#endif //DO NOT REMOVE
