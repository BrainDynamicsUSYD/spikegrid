/// \file
#include "typedefs.h"
//It is possible that we don't actually need any of the volatiles here.  The perf impact of leaving them shouldn't be too bad though
///used for storing arrays with their size.  Allows for the matlab_output (and other) function to take both the big and large arrays
typedef struct tagged_array{
    //we require volatile below as we don't want you to be able to write to an array using the pointer from the tagged array
    //however, other parts of the code could modify the underlying array, so use volatile to force reads
    const volatile Compute_float* const data; ///< the actual data
    const unsigned int size;                  ///< the total dimensions
    const unsigned int offset;                ///< offset (used by the gE and gI matrices
    const unsigned int subgrid;               ///< used when there is a subgrid within the grid - currently the only example is STDP.  The default value of this should be 1 - often not implemented
    const Compute_float minval;               ///< minimum value in array (for a colorbar - currently unused)
    const Compute_float maxval;               ///< maximum value in array (for a colorbar - currently unused)
} tagged_array;

typedef struct fcoords {Compute_float x;Compute_float y;} fcoords;
Compute_float* taggedarrayTocomputearray(const tagged_array input);
unsigned int __attribute__((const)) tagged_array_size_(const tagged_array in);
tagged_array* tagged_array_new(
        const volatile Compute_float* const data_, const unsigned int size_,
        const unsigned int offset_, const unsigned int subgrid_,
        const Compute_float minval_, const Compute_float maxval_);
fcoords* taggedArrayCOM(const tagged_array in);

Compute_float tagged_arrayMAX(const tagged_array in);
Compute_float tagged_arrayMIN(const tagged_array in);
