#include "parameters.h"
const float dt      = 0.1;                  //time step (ms)
const int duration  = 1000;                 //total duration(ms)
//TODO: make steps const
int steps           =     0   ;  //total number of steps

//Architecture properties
#define grid_size 100
//const int size      = 100;                  //grid size (size x size total neurons)
#define couplerange 15
//const int couplerange = 15;                   //adam calls this ext
const float WE      = 0.41;                 //excitatory coupling strength
const float sigE    = 14;                   //char. length for Ex symapses (int / float?)
const float WI      = 0.19;                 //Inhib coupling strength
const float sigI    = 42;                   //char. length for In synapses (int / float?)
const float SE      = 1;                    //amount to multiply Ex conns (question why not just use We?)
const float SI      = 2.55;                 //amount to multiply In conns (question why not just use In?)
const float syn     = 1;                    //synaptic strength multiplier (why not just We/Wi? - I am guessing some sort of convenience factor)

//Characteristic times (ms)
const float taurE   = 0.5;                  //Ex spike rise time
const float taudE   = 1.5;                  //Ex spike decay time
const float taurI   = 0.5;                  //In spike rise time
const float taudI   = 2.0;                  //In spike decay time
const int tref      = 5;                    //refractory time

//constants of potential difference
const float Vrt     = -70;                  //reset potential
const float Vth     = -55;                  //Threshold potential
const float Vlk     = -70;                  //leak reversal potential
const float Vex     = 0;                    //Ex reversal potential
const float Vin     = -80;                  //In reversal potential

//assorted other constants
const float rate    = 1;                    //Rate of external input (spikes/neuron/s)
const float glk     = 0.05;                 //leak conductance (ms^-1)


//some computed constants which are useful
int couple_array_size;
float* potentials;
float* potentials2;
