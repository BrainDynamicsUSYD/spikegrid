/// \file
#ifndef OUTPUT
#define OUTPUT
#include "enums.h"
#include "tagged_array.h"
typedef struct output_parameters output_parameters;
typedef struct model model;
typedef struct lagstorage lagstorage;


///The directory that we are outputting to
char outdir [100];
///The type of data to output
typedef enum {FLOAT_DATA=0,SPIKE_DATA=1} data_type;
///Holds data for outputtting in various ways
typedef struct output_s {
    const char name[10];            ///< a string identifier that is used to identify the output
    const data_type datatype;      ///< The type of data to output
    const union
    {
        const tagged_array TA_data;
        const lagstorage*  Lag_data;
    } data;                         ///< the data to return

} output_s; //used so that matlab has string identifiers that correspond to a specific tagged_array
Compute_float* taggedarrayTocomputearray(const tagged_array input);
void dooutput(const output_parameters* const m,const unsigned int t);
output_s __attribute__((pure)) getOutputByName(const char* const name);
void output_init(const model* const m);
void CleanupOutput();
output_s* Outputtable;
#endif //OUTPUT
