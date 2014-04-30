#include "paramheader.h"
//it is crucial that these parameters have exactly the same names as the various fields in the parameters object - otherwise you will break the parameter sweep function.
//it might also be a good idea to assign these values that never change with cross compatibilty with matlab
typedef enum {
                    dt,                                                         // time
                    WE,sigE,WI,sigI,SE,SI,                                      // couple
          //          ExR,ExD,InR,InD,tref,                                     // synapse
                    Vrt,Vth,Vlk,Vex,Vin,glk,                               //potential
                    stdp_limit,stdp_tau,stdp_strength,                          //STDP
                    U,D,F,                                                      //STD
                   // delay                                                       //movie
                   dummy          //Used for verification that nothing has been missed - DO NOT REMOVE
             } sweepabletypes;
typedef struct Sweepable
{
    const sweepabletypes type;
    const Compute_float minval;
    const Compute_float maxval;
    const unsigned int count;
} sweepable;
void testmodparam(const parameters input);
parameters* GetParamArray(const parameters input, const sweepable sweep);

