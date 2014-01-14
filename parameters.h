//these first few parameters actually escape into the paramheader file through magic
#define grid_size 100
#define couplerange 15
#ifndef PARAMATERS  //DO NOT REMOVE
#define PARAMATERS  //DO NOT REMOVE
//the following typedef must be before the include to get the right compute types
#include "paramheader.h"

////TODO: compiler does not warn on missing elements - fix
//Fun note - with the right optimisations GCC actually will pull these constants inline (for example disassemble evolvept_STDP with STDP off)
static const parameters Param = //the fact that this is static is a little messy - in theory gcc will create a copy for each .c file.  However - in reality, this doesn't appear to happen (perhaps GCC realises that it is const so that only one copy is required.  If the const is ever removed, static will cause incredibly weird behaviour.
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
        .SE      = 1.5,                  //amnt to multiply Ex conns; why not use We? (d=1.00; vary 1-1.5)
        .SI      = 2.3,                  //amnt to multiply In conns; why not use In? (d= 2.55; vary 2.3-4)
    },
    .synapse =
    {
        .Ex = {.R=0.5,.D=1.5},            //excitatory rise / decay time
        .In = {.R=0.5,.D=2.0},            //inhibitory rise / decay time
        .tref      = 5,                  //refractory time
    },
    .potential = 
    {
        .Vrt     = -70,                  //reset potential
        .Vth     = -55,                  //Threshold potential
        .Vlk     = -70,                  //leak reversal potential
        .Vex     = 0,                    //Ex reversal potential
        .Vin     = -80,                  //In reversal potential
        .glk     = 0.05,                 //leak current
        .rate = 1,
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
    .Movie = 
    {
        .Delay = 10,
    },
    .features =  //currently, features are global
    {
        .STDP       = OFF,
        .STD        = ON ,
        .Output     = OFF,
        .Movie      = OFF
        
    }
};
#endif //DO NOT REMOVE

