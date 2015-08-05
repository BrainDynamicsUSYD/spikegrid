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
///If you were storing many spike locations but on a small grid (for example very long spike histories, it would be possible to shrink this down to 8 bit integers
/// if you did that you would be limited to about a 100x100 grid and might get some overflow effects - need to check, but could reduce ram usage by 50%)
//limits - uint8_t -> 256 (might work sometimes), unit16_t ->65536 (we might have bigger problems here if we go beyond this)
typedef uint16_t Neuron_coord;
typedef struct coords {Neuron_coord x; /**<x coord*/Neuron_coord y;/**<y coord*/} coords;
///Useful constant to avoid messy conversions
static const Compute_float One = (Compute_float)1;
///Useful constant to avoid messy conversions
static const Compute_float Half = (Compute_float)0.5;
///Useful constant to avoid messy conversions
static const Compute_float Two = (Compute_float)2;
///Useful constant to avoid messy conversions
static const Compute_float Zero = (Compute_float)0;
#endif
