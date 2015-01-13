#ifndef RANDCONNS
#define RANDCONNS
#include "typedefs.h"
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

randconns_info init_randconns();
#endif
