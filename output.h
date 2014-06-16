/// \file
#ifndef OUTPUT
#define OUTPUT
#include "paramheader.h"
char outdir [100];
///Holds data for outputtting in various ways
typedef struct {
    const char name[10];      ///< a string identifier that is used to identify the output
    const tagged_array data;        ///< the data to return
    const Compute_float minval;     ///< minimum value in array (for a colorbar - currently unused)
    const Compute_float maxval;     ///< maximum value in array (for a colorbar - currently unused)
} output_s; //used so that matlab has string identifiers that correspond to a specific tagged_array
output_s* Outputtable;
Compute_float* taggedarrayTocomputearray(const tagged_array input);
void makemovie(const movie_parameters m,const unsigned int t);
output_s __attribute__((pure)) getOutputByName(const char* const name);
#endif //OUTPUT
