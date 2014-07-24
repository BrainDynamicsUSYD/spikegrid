/// \file
#include "typedefs.h"
#ifdef MATLAB
    #include <stdint.h>
    typedef uint16_t char16_t; //hack because mex.h is broken
    #include "mex.h" //matlab
    #include "matrix.h"  //matlab
    static inline mxClassID __attribute__((pure,const)) MatlabDataType()
    {
        return sizeof(Compute_float)==sizeof(float)?mxSINGLE_CLASS:mxDOUBLE_CLASS;
    }
#endif
