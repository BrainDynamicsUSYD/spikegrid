/// \file
#ifndef PARAMHEADER
#define PARAMHEADER
#ifdef FAST
///Used to enable simple switching between float and double
typedef float Compute_float ; //for speed
#else
///Used to enable simple switching between float and double
typedef double Compute_float ; //for accuracy
#endif
///used for storing arrays with their size.  Allows for the matlab_output (and other) function to take both the big and large arrays
typedef struct {
    //we require volatile below as we don't want you to be able to write to an array using the pointer from the tagged array
    //however, other parts of the code could modify the underlying array, so use volatile to force reads
    const volatile Compute_float* const data; ///< the actual data
    const unsigned int size;                  ///< the total dimensions
    const unsigned int offset;                ///< offset (used by the gE and gI matrices
} tagged_array;

///Simple enum for things that are on or off to make their state more obvious
typedef enum ON_OFF {OFF=0,ON=1} on_off;
///Normalization method to use when creating a coupling matrix
typedef enum NORM_TYPE {None=0,TotalArea=1,GlobalMultiplier=2,MultSep=3} Norm_type;
/// The normalization method used in the 2009 paper.  This method normalizes by the total area of Ex and In connections
typedef struct
{
    const Compute_float WE;
    const Compute_float WI;
} Total_area_parameters;
///Normalize by multiplying all connections by a constant
typedef struct
{
    const Compute_float GM; ///< The constant to multiply by
} global_multiplier_parameters;
///Used when normalizing excitatory and inhibitory seperately
typedef struct 
{
    const Compute_float Exfactor;
    const Compute_float Infactor;
} Multsep_parameters;
///Holds the parameters for the decay of a spike
typedef struct decay_parameters{
    const Compute_float R;  ///<rise time constant (units?)
    const Compute_float D;  ///<decay time constant (units?)
} decay_parameters;
///Enum to determine the type of connectivity
typedef enum ConnectType {HOMOGENEOUS=0,EXPONENTIAL=1} ConnectType;
///Enum to determine how many layers are in use
typedef enum LayerNumbers {SINGLELAYER=0,DUALLAYER=1} LayerNumbers;
///Enum to determine whether there is a recovery variable
typedef enum NEURON_TYPE {LIF=0,QIF=1,EIF=2} neuron_type;
///Parameters for a layer when it is the only one
typedef struct singlelayer_parameters
{
    const Compute_float WE       ;                  ///<excitatory coupling strength
    const Compute_float sigE   ;                    ///<char. length for Ex symapses (int / float?)
    const Compute_float WI     ;                    ///<Inhib coupling strength
    const Compute_float sigI   ;                    ///<char. length for In synapses (int / float?)
    const decay_parameters Ex;                      ///<Parameters for Ex connections
    const decay_parameters In;                      ///<Parameters for In connections
} singlelayer_parameters;
///Layer parameters for when there are two layers
typedef struct duallayer_parameters
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
    } Layer_parameters;                 
    const Norm_type     norm_type;      ///<what normalization method to use
    const union 
    {
        Total_area_parameters total_area;
        global_multiplier_parameters glob_mult;
        Multsep_parameters mult_sep;

    } normalization_parameters;         ///<holds data for different normalization methods
    const int tref     ;                ///<refractory time
} couple_parameters;
///Contains parameters which control the Voltage dynamics of neurons
typedef struct conductance_parameters
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
typedef struct recovery_parameters
{
    const Compute_float Wrt; /// reset of recovery variable
    const Compute_float Wir; /// determines if resonator or integrator
    const Compute_float Wcv; /// coupling with the voltage?
} recovery_parameters;
///Parameters for STDP
typedef struct STDP_parameters
{
    const Compute_float stdp_limit;     ///<maximum STDP that can be applied to a synapse as a fraction of its original value
    const Compute_float stdp_tau;       ///<tau for STDP (controls timescale of window function)
    const Compute_float stdp_strength;  ///< controls the amount of STDP
} STDP_parameters;
///Parameters controlling the shape of the STD recovery
typedef struct STD_parameters
{
    const Compute_float U;
    const Compute_float D;
    const Compute_float F;
} STD_parameters;
///Parameters for outputting movies
typedef struct movie_parameters
{
    const on_off MakeMovie;     ///< Are we making a movie?
    const unsigned int Output;  ///< What will be outputted in the movie
    const unsigned int Delay;   ///< how often to output it
} movie_parameters;
//Parameters for outputting spiking data
typedef struct output_parameters
{
    const on_off Switch;
    const char Filename[100];
} output_parameters;
///Parameters for a subthreshold wave (not necersarrily theta)
typedef struct theta_parameters
{
    const Compute_float strength;
	const Compute_float period;
} theta_parameters;

/// External input
typedef struct extinput
{
    const Compute_float gE0;
    const Compute_float gI0;
} extinput;

///Global switches to enable/disable features.  Also holds some model-independent parameters
typedef struct 
{
    const on_off Recovery;
	const on_off STDP;
    const on_off STD;
    const on_off Output;
    const on_off Theta;
    const Compute_float Timestep; ///< The timestep in the model
    const unsigned int   Simlength; ///< total number of timesteps to run
    const unsigned int Trial;
} model_features;
 
///Structure that holds all the parameters for a layer
typedef struct 
{
    const couple_parameters couple;
    const conductance_parameters potential;
    const STDP_parameters STDP;
    const STD_parameters STD;
    const movie_parameters Movie;
    const theta_parameters theta;
    const recovery_parameters recovery;
    const int skip;
    const output_parameters output;
} parameters;

// to specify an attribute to change for a yossarian run
typedef struct Sweepable
{
    const Compute_float minval;
    const Compute_float maxval;
    const int offset;
    const unsigned int count;
} sweepable;
///Useful constant to avoid messy conversions
static const Compute_float One = (Compute_float)1; 
///Useful constant to avoid messy conversions
static const Compute_float Half = (Compute_float)0.5;
///Useful constant to avoid messy conversions
static const Compute_float Two = (Compute_float)2;
///Useful constant to avoid messy conversions
static const Compute_float Zero = (Compute_float)0;
///ugly hack for recursive inclusion
#define PARAMETERS 
//get some macros for various sizes
#include "whichparam.h"  
///Size of the "large" arrays (notable examples are gE and gI)
#define conductance_array_size (grid_size + 2*couplerange)
///Size of a coupling matrix
#define couple_array_size (2*couplerange + 1)
#endif
