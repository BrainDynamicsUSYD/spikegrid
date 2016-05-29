/// \file
#include <stdlib.h>
#include <string.h>
#include "tagged_array.h"
unsigned int __attribute__((const,used)) tagged_array_size_(const tagged_array in) //something else affected by flto bug
{
    return in.size - (2*in.offset);
}
///Extracts the actual information out of a tagged array and converts it to a simple square matrix
Compute_float* __attribute((used)) taggedarrayTocomputearray(const tagged_array input)
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
    tagged_array* r = malloc(sizeof(*r)); //this leaks
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
Compute_float xbias_small(const volatile Compute_float* const data,const unsigned int smallsize)
{
    Compute_float ret = Zero;
    const int halfsize=(int)smallsize/2;
    for (unsigned int i=0;i<smallsize;i++)
    {
        for (unsigned int j=0;j<smallsize;j++)
        {
            ret += data[i*smallsize+j]*((int)j-halfsize);
        }
    }
    return ret;
}
#include <stdio.h>
tagged_array* taggedArrayXBias(const tagged_array* in)
{
    const unsigned int size = tagged_array_size_(*in);
    Compute_float* out = calloc(sizeof(Compute_float),size*size);
    Compute_float min = 0; //safe to start these at zero
    Compute_float max = 0;
 //   printf("smallsize is %i half is %i \n",in->subgrid,in->subgrid/2);
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            const volatile Compute_float* data =  &in->data[((i+in->offset)*in->size + j + in->offset)*in->subgrid*in->subgrid];
            out[i*size+j] = xbias_small(data,in->subgrid);
            if (out[i*size+j] > max) {max=out[i*size+j];}
            if (out[i*size+j] < min) {min=out[i*size+j];}
        }
    }
    printf("min is %f, max is %f\n",min,max);
    return tagged_array_new(out,size,0,1,min,max);
}
Compute_float tagged_arrayMAX(const tagged_array in)
{
    const unsigned int size = tagged_array_size_(in);
    Compute_float ret = 0;
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            if (in.data[i*size+j]>ret) {ret=in.data[i*size+j];}

        }
    }
    return ret;
}
Compute_float tagged_arrayMIN(const tagged_array in)
{
    const unsigned int size = tagged_array_size_(in);
    Compute_float ret = 0;
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            if (in.data[i*size+j]<ret) {ret=in.data[i*size+j];}

        }
    }
    return ret;
}
