/// \file
#ifndef OUTPUT
#define OUTPUT
#ifdef _WIN32
#include "VS\VS\Winheader.h"
#endif
#include "enums.h"
typedef struct model model;
typedef struct lagstorage lagstorage;
typedef struct tagged_array tagged_array;

///The directory that we are outputting to
extern char outdir [100];
///The type of data to output
typedef enum {FLOAT_DATA=0,SPIKE_DATA=1} data_type;
///Holds data for outputtting in various ways
typedef struct output_s {
    const char name[20];            ///< a string identifier that is used to identify the output
    const data_type datatype;      ///< The type of data to output
    const union
    {
        const tagged_array* TA_data;
        const lagstorage*  Lag_data;
    } data;                         ///< the data to return
    const on_off Updateable;
    tagged_array* (*UpdateFn) (const tagged_array* const in);
    const tagged_array* function_arg;
} output_s; //used so that matlab has string identifiers that correspond to a specific tagged_array
typedef struct overlaytext
{
    const char name[20];
    int (*func)();
} overlaytext;
void output_init(const model* const m);
void CleanupOutput();
#endif //OUTPUT
