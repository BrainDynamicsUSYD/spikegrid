#ifndef PARAMATERS
#define PARAMATERS
//How detailed is the  model and how long to run
extern const float dt   ;                   //time step (ms)
extern const int duration;                   //total duration(ms)
//TODO: make steps const
extern int steps            ; //total number of steps

//Architecture properties
#define grid_size 100
//const int size      = 100;                  //grid size (size x size total neurons)
#define couplerange 15
//const int couplerange = 15;                   //adam calls this ext
extern const float WE       ;               //excitatory coupling strength
extern const float sigE   ;                   //char. length for Ex symapses (int / float?)
extern const float WI     ;                 //Inhib coupling strength
extern const float sigI   ;                   //char. length for In synapses (int / float?)
extern const float SE      ;                    //amount to multiply Ex conns (question why not just use We?)
extern const float SI      ;                 //amount to multiply In conns (question why not just use In?)
extern const float syn     ;                    //synaptic strength multiplier (why not just We/Wi? - I am guessing some sort of convenience factor)

//Characteristic times (ms)
extern const float taurE  ;                  //Ex spike rise time
extern const float taudE   ;                  //Ex spike decay time
extern const float taurI   ;                  //In spike rise time
extern const float taudI   ;                  //In spike decay time
extern const int tref     ;                    //refractory time

//constants of potential difference
extern const float Vrt    ;                  //reset potential
extern const float Vth    ;                  //Threshold potential
extern const float Vlk     ;                  //leak reversal potential
extern const float Vex    ;                    //Ex reversal potential
extern const float Vin     ;                  //In reversal potential

//assorted other constants
extern const float rate   ;                    //Rate of external input (spikes/neuron/s)
extern const float glk    ;                 //leak conductance (ms^-1)


//some computed constants which are useful
extern int couple_array_size;
extern float* potentials;
extern float* potentials2;
#define conductance_array_size (grid_size + 2*couplerange)

//STDP related constants
extern const float stdp_limit;
extern const float  stdp_tau;
extern const float stdp_strength;

typedef enum ON_OFF {ON=0,OFF=1} on_off;

typedef struct model_features
{
	on_off STDP;
} model_features;


extern const model_features features;


//output constants
extern const on_off Output;
#endif
