/// \file
#ifndef OUTPUT
#define OUTPUT
typedef struct model model;
typedef struct lagstorage lagstorage;
typedef struct tagged_array tagged_array;

///The directory that we are outputting to
char outdir [100];
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
} output_s; //used so that matlab has string identifiers that correspond to a specific tagged_array
typedef struct overlaytext
{
    const char name[20];
    int (*func)();
} overlaytext;
output_s __attribute__((pure)) getOutputByName(const char* const name);
overlaytext* __attribute__((pure)) getOverlayByName(const char* const name);
void output_init(const model* const m);
void CleanupOutput();
output_s* Outputtable;
overlaytext* overlays;
#endif //OUTPUT
