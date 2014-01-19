#ifndef PARAMHEADER
#define PARAMHEADER
#ifdef FAST
typedef float Compute_float ; //for speed
#else
typedef double Compute_float ; //for accuracy
#endif
//making OFF 0 will turn off features by default
typedef enum ON_OFF {OFF=0,ON=1} on_off;
typedef struct time_parameters
{
    const Compute_float dt   ;                   //time step (ms)
} time_parameters;
typedef enum NORM_TYPE {None=0,TotalArea=1} Norm_type;
typedef struct
{
    const Compute_float WE;
    const Compute_float WI;
} Total_area_parameters;
typedef struct couple_parameters
{
    const Compute_float WE       ;               //excitatory coupling strength
    const Compute_float sigE   ;                   //char. length for Ex symapses (int / float?)
    const Compute_float WI     ;                 //Inhib coupling strength
    const Compute_float sigI   ;                   //char. length for In synapses (int / float?)
    const Compute_float SE      ;                    //amount to multiply Ex conns (question why not just use We?)
    const Compute_float SI      ;                 //amount to multiply In conns (question why not just use In?)
    const Norm_type     norm_type;
    const union 
    {
        Total_area_parameters total_area;
    } normalization_parameters;
} couple_parameters;

typedef struct decay_parameters{
    const Compute_float R;
    const Compute_float D;
} decay_parameters;

typedef struct synapse_parameters
{
    const decay_parameters Ex;
    const decay_parameters In;
    const int tref     ;                    //refractory time
} synapse_parameters;

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
    const int Delay;
} movie_parameters;
typedef struct model_features
{
	const on_off STDP; //enable / disable STDP (spike-timing dependent plasticity)
    const on_off STD;  //enable / disable STD  (short term depression)
    const on_off Output;
    const on_off Movie;
} model_features;


typedef struct parameters
{
    const time_parameters time;
    const couple_parameters couple;
    const synapse_parameters synapse;
    const conductance_parameters potential;
    const STDP_parameters STDP;
    const STD_parameters STD;
    const movie_parameters Movie;
    const model_features features;
} parameters;

static const Compute_float One = (Compute_float)1; //a useful constant so that you cna get a floating point 1 without needing a cast to float / double.  (the whole idea of compute_float is that it make switching 

#define PARAMETERS 
//get some macros for various sizes
#include "parameters.h" 
//these two get the underlying values from parameters.h and magic
#define conductance_array_size (grid_size + 2*couplerange)
#define couple_array_size (2*couplerange + 1)
#endif
