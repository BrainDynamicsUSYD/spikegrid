/// \file
#ifdef MATLAB
    #undef __STDC_UTF_16__ //This fixes a bug when trying to use a vaguely modern version of GCC with matlab.  Unfortunately it produces a warning
    #include "mex.h" //matlab
    #include "matrix.h"  //matlab
#endif
