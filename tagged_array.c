/// \file
#include <stdlib.h>
#include "tagged_array.h"
unsigned int __attribute__((const)) tagged_array_size(const tagged_array in)
{
    return in.size - (2*in.offset);
}
///Extracts the actual information out of a tagged array and converts it to a simple square matrix
Compute_float* taggedarrayTocomputearray(const tagged_array input)
{
    const unsigned int size = tagged_array_size(input);
    Compute_float * ret = calloc(sizeof(*ret),size*size*input.subgrid*input.subgrid);
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            for (unsigned int k=0;k<input.subgrid;k++)
            {
                for (unsigned int l=0;l<input.subgrid;l++)
                {
                    //this part reshuffles the matrix so that it looks better when you do a plot in matlab.  The subgrid stuff is mainly used for STDP where there is a matrix associated with each point
                    const Compute_float val =  input.data[((i+input.offset)*input.size + j + input.offset)*input.subgrid*input.subgrid + k*input.subgrid + l ];
                    ret[(i*input.subgrid+k)*size*input.subgrid +j*input.subgrid+l]=val;//this at least appears to bee correct
                }
            }
        }
    }
    return ret;
}

