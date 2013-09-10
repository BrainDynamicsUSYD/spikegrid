#ifndef PARAMATERS
#define PARAMATERS
//TODO: compiler does not warn on unitialized variables
typedef struct time_parameters
{
    const float dt   ;                   //time step (ms)
} time_parameters;



//Architecture properties
#define grid_size 100
//const int size      = 100;                  //grid size (size x size total neurons)
#define couplerange 15

//const int couplerange = 15;                   //adam calls this ext
typedef struct couple_parameters
{
const float WE       ;               //excitatory coupling strength
const float sigE   ;                   //char. length for Ex symapses (int / float?)
const float WI     ;                 //Inhib coupling strength
const float sigI   ;                   //char. length for In synapses (int / float?)
const float SE      ;                    //amount to multiply Ex conns (question why not just use We?)
const float SI      ;                 //amount to multiply In conns (question why not just use In?)
const float syn     ;                    //synaptic strength multiplier (why not just We/Wi? - I am guessing some sort of convenience factor)
} couple_parameters;

typedef struct synapse_parameters
{
const float taurE  ;                  //Ex spike rise time
const float taudE   ;                  //Ex spike decay time
const float taurI   ;                  //In spike rise time
const float taudI   ;                  //In spike decay time
const int tref     ;                    //refractory time
} synapse_parameters;

typedef struct potential_parameters
{
const float Vrt    ;                  //reset potential
const float Vth    ;                  //Threshold potential
const float Vlk     ;                  //leak reversal potential
const float Vex    ;                    //Ex reversal potential
const float Vin     ;                  //In reversal potential
} potential_parameters;

typedef struct misc_parameters //TODO: really need to rename this
{
const float rate   ;                    //Rate of external input (spikes/neuron/s)
const float glk    ;                 //leak conductance (ms^-1)
} misc_parameters;
typedef struct STDP_parameters
{
    const float stdp_limit;
    const float  stdp_tau;
    const float stdp_strength;
} STDP_parameters;
typedef struct STD_parameters
{
    const float U;
    const float D;
    const float F;
} STD_parameters;

//some computed constants which are useful
int couple_array_size;
float* potentials;
float* potentials2;
#define conductance_array_size (grid_size + 2*couplerange)

//making OFF 0 will turn off features by default
typedef enum ON_OFF {OFF=0,ON=1} on_off;

typedef struct model_features
{
	const on_off STDP; //enable / disable STDP (spike-timing dependent plasticity)
    const on_off STD;  //enable / disable STD  (short term depression)
} model_features;

//output constants
extern const on_off Output;

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
extern const parameters Param;
#endif
