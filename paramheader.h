typedef struct time_parameters
{
    const Compute_float dt   ;                   //time step (ms)
} time_parameters;
typedef struct couple_parameters
{
const Compute_float WE       ;               //excitatory coupling strength
const Compute_float sigE   ;                   //char. length for Ex symapses (int / float?)
const Compute_float WI     ;                 //Inhib coupling strength
const Compute_float sigI   ;                   //char. length for In synapses (int / float?)
const Compute_float SE      ;                    //amount to multiply Ex conns (question why not just use We?)
const Compute_float SI      ;                 //amount to multiply In conns (question why not just use In?)
const Compute_float syn     ;                    //synaptic strength multiplier (why not just We/Wi? - I am guessing some sort of convenience factor)
} couple_parameters;

typedef struct synapse_parameters
{
const Compute_float taurE  ;                  //Ex spike rise time
const Compute_float taudE   ;                  //Ex spike decay time
const Compute_float taurI   ;                  //In spike rise time
const Compute_float taudI   ;                  //In spike decay time
const int tref     ;                    //refractory time
} synapse_parameters;

typedef struct potential_parameters
{
const Compute_float Vrt    ;                  //reset potential
const Compute_float Vth    ;                  //Threshold potential
const Compute_float Vlk     ;                  //leak reversal potential
const Compute_float Vex    ;                    //Ex reversal potential
const Compute_float Vin     ;                  //In reversal potential
} potential_parameters;

typedef struct misc_parameters //TODO: really need to rename this
{
const Compute_float rate   ;                    //Rate of external input (spikes/neuron/s)
const Compute_float glk    ;                 //leak conductance (ms^-1)
} misc_parameters;
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
//making OFF 0 will turn off features by default
typedef enum ON_OFF {OFF=0,ON=1} on_off;

typedef struct model_features
{
	const on_off STDP; //enable / disable STDP (spike-timing dependent plasticity)
    const on_off STD;  //enable / disable STD  (short term depression)
} model_features;

//output constants

typedef struct parameters
{
    const time_parameters time;
    const couple_parameters couple;
    const synapse_parameters synapse;
    const potential_parameters potential;
    const misc_parameters misc;
    const STDP_parameters STDP;
    const STD_parameters STD;
    const model_features features;
} parameters;
