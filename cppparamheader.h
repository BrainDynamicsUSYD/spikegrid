/// \file
#ifndef CPPPARAM
#define CPPPARAM
//c++ can't read all of the paramheader file, so this file gets its own entry
typedef struct output_parameters
{
    const output_method method;  ///< Are we outputting something
    const unsigned int Output;          ///< What will be outputted
    const unsigned int Delay;           ///< how often to output it
} output_parameters;
typedef struct
{
    const Compute_float timeperiod;
    const Compute_float lag;
} Stimulus_parameters;
#endif
