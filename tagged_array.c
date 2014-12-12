/// \file
#include <stdlib.h>
#include <string.h>
#include "tagged_array.h"
unsigned int __attribute__((const)) tagged_array_size_(const tagged_array in)
{
    return in.size - (2*in.offset);
}
///Extracts the actual information out of a tagged array and converts it to a simple square matrix
Compute_float* taggedarrayTocomputearray(const tagged_array input)
{
    const unsigned int size = tagged_array_size_(input);
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
///look I can write a constructor for an object in C
tagged_array* tagged_array_new(const volatile Compute_float* const data_, const unsigned int size_, const unsigned int offset_, const unsigned int subgrid_, const Compute_float minval_, const Compute_float maxval_)
{
    tagged_array T = {.data=data_,.size=size_,.offset=offset_,.subgrid=subgrid_,.minval=minval_,.maxval=maxval_};
    tagged_array* r = malloc(sizeof(*r));
    memcpy(r,&T,sizeof(*r));
    return r;
}

fcoords COM_small(const volatile Compute_float* const data,const unsigned int smallsize)
{
    fcoords ret = {.x=Zero, .y=Zero};
    Compute_float sum = Zero;
    for (unsigned int i=0;i<smallsize;i++)
    {
        for (unsigned int j=0;j<smallsize;j++)
        {
            ret.x += data[i*smallsize+j]*i;
            ret.x += data[i*smallsize+j]*j;
            sum += data[i*smallsize+j];
        }
    }
    ret.x -= sum*smallsize;
    ret.y -= sum*smallsize;
    return ret;
}

fcoords* taggedArrayCOM(const tagged_array in)
{
    const unsigned int size = tagged_array_size_(in);
    fcoords* out = calloc(sizeof(fcoords),size*size);
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {

            const volatile Compute_float* data =  &in.data[((i+in.offset)*in.size + j + in.offset)*in.subgrid*in.subgrid];
            out[i*size+j] = COM_small(data,in.subgrid);
        }
    }
    return out;
}
