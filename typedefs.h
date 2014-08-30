/// \file
#ifndef TYPEDEFS
#define TYPEDEFS
#include <stdint.h>
#ifdef FAST
///Used to enable simple switching between float and double
typedef float Compute_float ; //for speed
#else
///Used to enable simple switching between float and double
typedef double Compute_float ; //for accuracy
#endif
///store coordinates in 16-bit integers to limit the memory usage
typedef int16_t Neuron_coord;

///Useful constant to avoid messy conversions
static const Compute_float One = (Compute_float)1;
///Useful constant to avoid messy conversions
static const Compute_float Half = (Compute_float)0.5;
///Useful constant to avoid messy conversions
static const Compute_float Two = (Compute_float)2;
///Useful constant to avoid messy conversions
static const Compute_float Zero = (Compute_float)0;
#endif
