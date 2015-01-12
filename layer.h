/// \file
#ifndef LAYER
#define LAYER
#include "lagstorage.h"
#include "typedefs.h"
#include "enums.h" //needed at least for the is-inhibitory flag
typedef struct STD_data STD_data; //forward declare STD_data to make things cleaner - makes this file a little messier, but it makes it more obvious where things come from
///hold the requisite data for a layer that enables it to be evolved through time.
typedef struct parameters parameters;
typedef struct STDP_data STDP_data;
typedef struct coords {Neuron_coord x; /**<x coord*/Neuron_coord y;/**<y coord*/} coords;
typedef struct randomconnection
{
    Compute_float   strength;
    Compute_float   stdp_strength;
    coords          destination;
} randomconnection;

typedef struct randconns_info
{
    randomconnection* randconns;                ///<stores random connections
    randomconnection** randconns_reverse;   //reverse connections (might not be required?)
    unsigned int* rev_pp;                   //no of to conns / point
    randomconnection*** randconns_reverse_lookup; //lookup to randconns_reverse - triple pointers are fun

} randconns_info;
//making some of these arrays fixed rather than pointers would be nice
//it would probably improve cache access.
//However, there would be issues - some functions pass layer rather than layer* which
//would cause problems (segfaults that are stack overflows)
typedef struct layer
{
    Compute_float* const connections;     ///<Matrix of connections coming from a single point
    Compute_float* voltages;                    ///<Input voltages
    Compute_float* voltages_out;                ///<return value
    Compute_float* recoverys;                   ///<Recovery variable
    Compute_float* recoverys_out;               ///<Return value for recovery variable
    Compute_float* const Extimecourse;    ///<store time course of Ex synapses
    Compute_float* const Intimecourse;    ///<store time course of In synapses
    Compute_float* const Mytimecourse;    ///<store time course of synapses in dual layer case
    randconns_info rcinfo;
    lagstorage      firinglags;
    STDP_data*      STDP_data;
    parameters*     P;                              ///<The parameters that we used to make the layer
    STD_data*       std;                               ///<Some info that is needed for STD
    const on_off Layer_is_inhibitory;
} layer;
#endif
