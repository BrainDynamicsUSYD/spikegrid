/// \file
#ifndef PARAMHEADER
#define PARAMHEADER
#include "typedefs.h"
#include "enums.h"
#include "cppparamheader.h" //I am not a huge fan of this particular include - but unfortuneately it is the simplest solutionit is the simplest solutiontypedef struct
typedef struct
{
    const Compute_float WE; ///<total strength of excitatory connections
    const Compute_float WI; ///<total strength of inhibitory connections
} Total_area_parameters;
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
///Holds the parameters for the decay of a spike
typedef struct decay_parameters{
    const Compute_float R;  ///<rise time constant (units?)
    const Compute_float D;  ///<decay time constant (units?)
} decay_parameters;
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
///Contains parameters which control the Voltage dynamics of neurons
typedef struct
{
    const struct
    {
        const neuron_type type ; ///< type of neuron - how it integrates input
        const union
        {
            const struct {const Compute_float Vth;} QIF;
            const struct {const Compute_float Vth; const Compute_float Dpk;} EIF;
        } extra;
    } type                     ;  ///< holds parameters for how to integrate neuron
    const Compute_float Vrt    ;  ///<Reset potential.
    const Compute_float Vpk    ;  ///<Peak potential
    const Compute_float Vlk    ;  ///<leak reversal potential
    const Compute_float Vex    ;  ///<Ex reversal potential
    const Compute_float Vin    ;  ///<In reversal potential
    const Compute_float glk    ;  ///<leak conductance (ms^-1)
    const Compute_float rate   ;  ///<Rate of external input (spikes/neuron/s)
} conductance_parameters;
/// Contains parameters which control the Recovery dynamics of neurons
typedef struct
{
    const Compute_float Wrt; ///< reset of recovery variable
    const Compute_float Wir; ///< determines if resonator or integrator
    const Compute_float Wcv; ///< coupling with the voltage?
} recovery_parameters;
///Parameters for STDP
typedef struct STDP_parameters
{
    const Compute_float stdp_limit;     ///<maximum STDP that can be applied to a synapse as a fraction of its original value
    const Compute_float stdp_tau;       ///<tau for STDP (controls timescale of window function)
    const Compute_float stdp_strength;  ///< controls the amount of STDP
    const on_off        STDP_on;
} STDP_parameters;
///Parameters controlling the shape of the STD recovery
///see various markram papers for details
typedef struct STD_parameters
{
    const Compute_float U;  ///<Utilisation
    const Compute_float D;  ///<depression timescale
    const Compute_float F;  ///<facilitation timescale
} STD_parameters;

///Parameters for a subthreshold wave (not necersarrily theta)
typedef struct theta_parameters
{
    const Compute_float strength;   ///<the strength of the theta wave
  	const Compute_float period;     ///<the period of the theta wave
} theta_parameters;

typedef struct randconn_parameters
{
    const unsigned int numberper;
    const Compute_float str;
} randconn_parameters;

/// External input
typedef struct
{
    const Compute_float gE0;    ///< constant input to gE
    const Compute_float gI0;    ///< constant input to gI
} extinput;



struct Job;
///Within a single run of the program it is possible to run multiple different tasks.
///For example, you can run several jobs with random initial conditions,
///or try a few specified initial conditions.
///currently used mainly for testing
typedef struct Job
{
    const InitConds initcond; ///< the initial condition
    const Compute_float Voltage_or_count; ///< some info for the initial condition
    const struct Job* const next; ///< this is a linked list, so treat it like one
} Job;
///Global switches to enable/disable features.  Also holds some model-independent parameters
typedef struct model_features
{
    const on_off Recovery;          ///< Is recovery enables
	const on_off STDP;              ///< Is STDP enabled
    const on_off STD;               ///< Is STD enabled
    const on_off Theta;             ///< Is theta (subthreshold oscillation enabled
    const on_off Random_connections;///< Is random connectivity enabled?
    const Compute_float Timestep;   ///< The timestep in the model
    const unsigned int Simlength;   ///< total number of timesteps to run
    const char Outprefix[100];      ///< Sweep prefix
    const Job job;                  ///< the jobs we are going to run
    const on_off ImageStim;         ///< whether to use an image based stimulus.
    const on_off Disablewrapping;   ///< disable wrapping in the model.  TODO: could be implemented in a more efficient way.  Currently just
    const on_off LocalStim;         ///< Is the local stimulus on
    const output_parameters output[10];         ///<What things do you want to output 10 outputs should be enough (but limit is arbitrary)
    const on_off UseAnimal;
} model_features;

///Structure that holds all the parameters for a layer
typedef struct parameters
{
    const couple_parameters couple;             ///<Parameters controlling coupling and synapses
    const conductance_parameters potential;     ///<parameters controlling the potential; dynamics of neurons
    const STDP_parameters STDP;                 ///<parameters controlling STDP
    const STD_parameters STD;                   ///<parameters controlling STD
    const theta_parameters theta;               ///<parameters controling theta wave
    const recovery_parameters recovery;         ///<parameters controlling recovery (alternative to refractory time)
    const int skip;                             ///<Number of neurons to skip over in the layer setting skip=2 will produce 4-1 ratio of neurons to a layer with skip=1
    const randconn_parameters random;           ///<parameters controlling random connectivity
    const Stimulus_parameters Stim;             ///< Holds parameters for localized stimulus
} parameters;

/// On yossarian this is used for a parameter sweep
typedef struct sweepable
{
    const Compute_float minval; ///< the minimum value of the parameter
    const Compute_float maxval; ///< the maximum value of the parameter
    const int offset;           ///< the offset into the parameters object of the value to modify
    const unsigned int count;   ///< the number of jobs to create - spacing of parameter is linear between maxval and minval
    const on_off SweepEx;
    const on_off SweepIn;
} sweepable;
#include "whichparam.h"
#endif
