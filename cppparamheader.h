/// \file
#ifndef CPPPARAM
#define CPPPARAM
#include "enums.h"
//c++ can't read all of the paramheader file, so this file gets its own entry
//THIS IS A MASSIVE HACK - TODO: FIXME
typedef struct output_parameters
{
    const output_method method;         ///< Are we outputting something
    const unsigned int Output;          ///< What will be outputted
    const unsigned int Delay;           ///< how often to output it
    const char Overlay[20];
} output_parameters;
typedef struct
{
    const Compute_float timeperiod;
    const Compute_float lag;
    const char ImagePath[100];          ///< Path to use for image stimulus
    const Compute_float  PreconditioningTrials;   ///< Number of preconditioning trials
} Stimulus_parameters;
#endif
