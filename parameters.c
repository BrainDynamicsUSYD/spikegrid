#include "parameters.h"
//TODO: compiler does not warn on missing elements - fix
const parameters Param =
{
    .time =
    {
        .dt=0.1
    },
    .couple =
    {
        .WE      = 0.41,                 //excitatory coupling strength
        .sigE    = 14,                   //char. length for Ex symapses (int / float?)
        .WI      = 0.19,                 //Inhib coupling strength
        .sigI    = 42,                   //char. length for In synapses (int / float?)
        .SE      = 1,                    //amount to multiply Ex conns (question why not just use We?)
        .SI      = 2.55,                 //amount to multiply In conns (question why not just use In?)
        .syn     = 1,    
    },
    .synapse =
    {
        .taurE   = 0.5,                  //Ex spike rise time
        .taudE   = 1.5,                  //Ex spike decay time
        .taurI   = 0.5,                  //In spike rise time
        .taudI   = 2.0,                  //In spike decay time
        .tref      = 5,                    //refractory time

    },
    .potential = 
    {
        .Vrt     = -70,                  //reset potential
        .Vth     = -55,                  //Threshold potential
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
    },
    .misc =
    {
        .rate = 1,
        .glk  = 0.05
    },
    .STDP = 
    {
        .stdp_limit     = 0.1,
        .stdp_tau       = 20,
        .stdp_strength  = 0.0004
    }, 
    .STD =
    {
        .U  = 0.5,
        .D  = 0.11,
        .F  = 0.005
    },
    .features =
    {
        .STDP    = OFF,
        .STD     = OFF
    }
};


//some computed constants which are useful
int couple_array_size;
float* potentials;
float* potentials2;


//features
const on_off Output = ON;
