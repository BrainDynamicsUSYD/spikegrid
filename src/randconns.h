#ifndef RANDCONNS
#define RANDCONNS
#include "typedefs.h"
#include "paramheader.h"
typedef struct randconn_parameters randconn_parameters;
typedef struct randomconnection
{
    Compute_float   strength;
    Compute_float   stdp_strength;
    coords          destination;
    coords          source;
} randomconnection;
typedef struct randconns_info
{
    randomconnection* randconns;                ///<stores random connections
    randomconnection** randconns_reverse;   //reverse connections (might not be required?)
    unsigned int* rev_pp;                   //no of to conns / point - can vary due to randomness
    const unsigned int numberper;           //no of conns leaving point - essentially fixed except in special cases
    const unsigned int nospecials;
    const on_off UsingFancySpecials;
    unsigned int SpecialAInd;
    unsigned int SpecialBInd;
    randomconnection* Aconns;
    randomconnection* Bconns;
} randconns_info;

randconns_info* init_randconns(const randconn_parameters rparam,const couple_parameters couple);

randomconnection** GetRandomConnsArriving(const coords coord, const randconns_info rcinfo, unsigned int* numberconns);
randomconnection* GetRandomConnsLeaving  (const coords coord, const randconns_info rcinfo, unsigned int* numberconns);
#endif
