#ifndef PARAMATERS
#define PARAMATERS
//the following typedef must be before the include to get the right compute types
//typedef float Compute_float ; //for speed
typedef double Compute_float ; //for accuracy
#include "paramheader.h"

//Architecture properties
#define grid_size 100
#define couplerange 15
#define conductance_array_size (grid_size + 2*couplerange)

//some computed constants which are useful
int couple_array_size;
Compute_float* potentials;
Compute_float* potentials2;

////TODO: compiler does not warn on missing elements - fix
//Fun note - with the right optimisations GCC actually will pull these constants inline (for example disassemble evolvept_STDP with STDP off)
static const parameters Param =
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
        .SE             = 1.5,           //amnt to multiply Ex conns; why not use We? (d=1.00; vary 1-1.5)
        .SI             = 2.3,           //amnt to multiply In conns; why not use In? (d= 2.55; vary 2.3-4)
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
        .STD            = ON             
    }
};
static const on_off Output = ON;
#endif

