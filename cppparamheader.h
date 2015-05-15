/// \file
#ifndef CPPPARAM
#define CPPPARAM
#include "typedefs.h"
#include "enums.h"
//c++ can't read all of the paramheader file, so this file gets its own entry
//THIS IS A MASSIVE HACK - TODO: FIXME
typedef struct output_parameters
{
    const output_method method;         ///< Are we outputting something
    const unsigned int Output;          ///< What will be outputted
    const unsigned int Delay;           ///< how often to output it
    const char Overlay[20];
} output_parameters;
///Holds the parameters for the decay of a spike
typedef struct decay_parameters{
    const Compute_float R;  ///<rise time constant (units?)
    const Compute_float D;  ///<decay time constant (units?)
    const Synaptic_evol synfun;
} decay_parameters;
typedef struct
{
    const Compute_float timeperiod;
    const Compute_float lag;
    const char ImagePath[100];          ///< Path to use for image stimulus
    const Compute_float  PreconditioningTrials;   ///< Number of preconditioning trials
    const Compute_float NoUSprob;
    const on_off Testing;
    const Compute_float Prob1;
    const on_off TestPathChoice;
    const on_off Periodic;
    const Compute_float I1;
    const Compute_float I0;
} Stimulus_parameters;
///Parameters for a layer when it is the only one
typedef struct
{
    const Compute_float WE       ;                  ///<excitatory coupling strength
    const Compute_float sigE   ;                    ///<char. length for Ex symapses (int / float?)
    const Compute_float WI     ;                    ///<Inhib coupling strength
    const Compute_float sigI   ;                    ///<char. length for In synapses (int / float?)
    const decay_parameters Ex;                      ///<Parameters for Ex connections
    const decay_parameters In;                      ///<Parameters for In connections
} singlelayer_parameters;
///Layer parameters for when there are two layers
typedef struct
{
    const ConnectType       connectivity;   ///<Connectivity type: EXPONENTIAL or HOMOGENEOUS
    const Compute_float     W;              ///<Maximum connectivity strength
    const Compute_float     sigma;      ///<Connectivity decay length scale
    const decay_parameters  synapse;    ///<Parameters of spike
} duallayer_parameters;
///Normalize by multiplying all connections by a constant
typedef struct
{
    const Compute_float GM; ///< The constant to multiply by
} global_multiplier_parameters;
///Used when normalizing excitatory and inhibitory seperately
typedef struct
{
    const Compute_float Exfactor;   ///< Factor to multiply excitatory connections by
    const Compute_float Infactor;   ///< Factor to multiply inhibitory connections by
} Multsep_parameters;
typedef struct
{
    const Compute_float WE; ///<total strength of excitatory connections
    const Compute_float WI; ///<total strength of inhibitory connections
} Total_area_parameters;
/// Contains parameters about coupling within either a single or dual layer
typedef struct couple_parameters
{
    const LayerNumbers Layertype;       ///<Whether we are using a single/or dual layer model
    const union
    {
        singlelayer_parameters single;  ///<single layer
        duallayer_parameters   dual;    ///<double layer
    } Layer_parameters;                 ///<Parameters specific to the number of layers
    const Norm_type     norm_type;      ///<what normalization method to use
    const union
    {
        Total_area_parameters total_area;
        global_multiplier_parameters glob_mult;
        Multsep_parameters mult_sep;

    } normalization_parameters;         ///<holds data for different normalization methods
    const Compute_float tref     ;                ///<refractory time
} couple_parameters;
#endif
