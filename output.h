/// \file
#ifndef OUTPUT
#define OUTPUT
#include "layer.h"
///used for storing arrays with their size.  Allows for the matlab_output (and other) function to take both the big and large arrays
typedef struct {
    //we require volatile below as we don't want you to be able to write to an array using the pointer from the tagged array
    //however, other parts of the code could modify the underlying array, so use volatile to force reads
    const volatile Compute_float* const data; ///< the actual data
    const unsigned int size;                  ///< the total dimensions
    const unsigned int offset;                ///< offset (used by the gE and gI matrices
    const Compute_float minval;               ///< minimum value in array (for a colorbar - currently unused)
    const Compute_float maxval;               ///< maximum value in array (for a colorbar - currently unused)
} tagged_array;


char outdir [100];
typedef enum {FLOAT_DATA=0,RINGBUFFER_DATA=1} data_type;
///Holds data for outputtting in various ways
typedef struct {
    const char name[10];            ///< a string identifier that is used to identify the output
    const data_type data_type;
    const union test
    {   
        const tagged_array TA_data;        
        const ringbuffer* const  RB_data;
    } data;                         ///< the data to return

} output_s; //used so that matlab has string identifiers that correspond to a specific tagged_array
Compute_float* taggedarrayTocomputearray(const tagged_array input);
void dooutput(const output_parameters* const m,const unsigned int t);
output_s __attribute__((pure)) getOutputByName(const char* const name);
void output_init(const model* const m);
#endif //OUTPUT
