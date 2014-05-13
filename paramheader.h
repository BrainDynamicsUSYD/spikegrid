
#ifndef PARAMHEADER
#define PARAMHEADER
#ifdef FAST
typedef float Compute_float ; //for speed
#else
typedef double Compute_float ; //for accuracy
#endif
//used for storing arrays with their size.  Allows for the matlab_output function to take both the big and large arrays
typedef struct {
    //we require volatile below as we don't want you to be able to write to an array using the pointer from the tagged array
    //however, other parts of the code could modify the underlying array, so use volatile to force reads
    const volatile Compute_float* const data;
    const unsigned int size;
    const unsigned int offset;
} tagged_array;
typedef struct {
    const char const name[10];
    const tagged_array data;
    const Compute_float minval;
    const Compute_float maxval;
} output_s; //used so that matlab has string identifiers that correspond to a specific tagged_array


//making OFF 0 will turn off features by default
typedef enum ON_OFF {OFF=0,ON=1} on_off;
//structs to store various parameters
typedef struct time_parameters
{
    const Compute_float dt   ;                   //time step (ms)
} time_parameters;
typedef enum NORM_TYPE {None=0,TotalArea=1,GlobalMultiplier=2,MultSep=3} Norm_type;
typedef struct
{
    const Compute_float WE;
    const Compute_float WI;
} Total_area_parameters;
typedef struct
{
    const Compute_float GM;
} global_multiplier_parameters;
typedef struct 
{
    const Compute_float Exfactor;
    const Compute_float Infactor;
} Multsep_parameters;
typedef struct decay_parameters{
    const Compute_float R;
    const Compute_float D;
} decay_parameters;
typedef enum LayerNumbers {SINGLELAYER=0,DUALLAYER=1} LayerNumbers;
typedef struct singlelayer_parameters
{
    const Compute_float WE       ;                  //excitatory coupling strength
    const Compute_float sigE   ;                    //char. length for Ex symapses (int / float?)
    const Compute_float WI     ;                    //Inhib coupling strength
    const Compute_float sigI   ;                    //char. length for In synapses (int / float?)
    const decay_parameters Ex;
    const decay_parameters In;
} singlelayer_parameters;
typedef struct duallayer_parameters
{
    const Compute_float     W; //basically as for the singlelayer_properties but with some features missing
    const Compute_float     sigma;
    const decay_parameters  synapse;
} duallayer_parameters;
typedef struct couple_parameters
{
    const LayerNumbers Layertype;
    const union
    {
        singlelayer_parameters single;
        duallayer_parameters   dual;
    } Layer_parameters;
    const Norm_type     norm_type;                  //what normalization method to use
    const union 
    {
        Total_area_parameters total_area;
        global_multiplier_parameters glob_mult;
        Multsep_parameters mult_sep;

    } normalization_parameters;                   //holds data for different normalization methods
    const int tref     ;                    //refractory time
} couple_parameters;




typedef struct conductance_parameters
{
    const Compute_float Vrt    ;                  //reset potential
    const Compute_float Vth    ;                  //Threshold potential
    const Compute_float Vlk     ;                  //leak reversal potential
    const Compute_float Vex    ;                    //Ex reversal potential
    const Compute_float Vin     ;                  //In reversal potential
    const Compute_float glk    ;                 //leak conductance (ms^-1)
    const Compute_float rate   ;                    //Rate of external input (spikes/neuron/s)
} conductance_parameters;

typedef struct STDP_parameters
{
    const Compute_float stdp_limit;
    const Compute_float  stdp_tau;
    const Compute_float stdp_strength;
} STDP_parameters;
typedef struct STD_parameters
{
    const Compute_float U;
    const Compute_float D;
    const Compute_float F;
} STD_parameters;
typedef struct movie_parmeters
{
    const on_off MakeMovie;
    const output_s output;
    const unsigned int Delay;
} movie_parameters;
typedef struct theta_parameters
{
    const Compute_float strength;
	const Compute_float period;
} theta_parameters;
typedef struct model_features
{
	const on_off STDP;
    const on_off STD;  //enable / disable STD  (short term depression)
    const on_off Output;
    const on_off Theta;
    const Compute_float Timestep;
} model_features;

/// procedure for adding new parameters.
/// 1. Add relevant parameter to the parameters struct
/// 2. Add entry to the sweepabletypes enum
/// 3. Update the modparam function in newparam.c to copy your new parameter
/// 4. Add a new default value in parameters.h (this should probably be with the feature off)
typedef struct parameters
{
    const couple_parameters couple;
    const conductance_parameters potential;
    const STDP_parameters STDP;
    const STD_parameters STD;
    const movie_parameters Movie;
    const theta_parameters theta;
} parameters;
///it is crucial that these parameters have exactly the same names as the various fields in the parameters object.  otherwise you will break the parameter sweep function.
///it might also be a good idea to assign these values that never change with cross compatibilty with matlab
///
///Many parameters are currently not supported by this - need to improve, but basic framework is there
typedef enum {
         //           WE,sigE,WI,sigI,SE,SI,                                      // couple
          //          ExR,ExD,InR,InD,tref,                                     // synapse
                    Vrt,Vth,Vlk,Vex,Vin,glk,                               //potential
                    stdp_limit,stdp_tau,stdp_strength,                          //STDP
           //         U,D,F,                                                      //STD
                   // delay                                                       //movie
                   dummy          //Used for verification that nothing has been missed - DO NOT REMOVE
             } sweepabletypes;
typedef struct Sweepable
{
    const sweepabletypes type;
    const Compute_float minval;
    const Compute_float maxval;
    const unsigned int count;
} sweepable;

//some useful constants
static const Compute_float One = (Compute_float)1; //a useful constant so that you cna get a floating point 1 without needing a cast to float / double.  (the whole idea of compute_float is that it make switching 
static const Compute_float Half = (Compute_float)0.5;
static const Compute_float Two = (Compute_float)2;
static const Compute_float Zero = (Compute_float)0;
#define PARAMETERS 
//get some macros for various sizes
#include "parameters.h" 
//these two get the underlying values from parameters.h and magic
#define conductance_array_size (grid_size + 2*couplerange)
#define couple_array_size (2*couplerange + 1)
#endif
