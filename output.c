#include "pixeltypes.h"
#include "taggedarray.h"
#include "output.h"
int rescalefloat (const Compute_float in,const Compute_float maxval, const Compute_float minval)
{
    return (int)((in - minval)/(maxval-minval)*255.0);
}
bitmap_t* FloattoBitmap(const tagged_array input,const Compute_float maxval, const Compute_float minval)
{
    const int size = input.size - (2*input.offset);
    bitmap_t* bp = (bitmap_t*)malloc(sizeof(bitmap_t));
	bitmap_t b = *bp;
    b.pixels = malloc(sizeof(pixel_t)*input.size*input.size);
    b.width=input.size;
    b.height=input.size;

    for (int i=0;i<input.size;i++)
    {
        for (int j=0;j<input.size;j++)
        {
            const Compute_float val =  input.data[(i+input.offset)*input.size + j + input.offset ];
            if (val > maxval) 
            {
                b.pixels[i*size+j].red = 255;
                b.pixels[i*size+j].green = 255;
                b.pixels[i*size+j].blue = 255;
            }
            else
            {
                b.pixels[i*size+j].red = rescalefloat(val,maxval,minval);
                b.pixels[i*size+j].green = 0;
                b.pixels[i*size+j].blue = 0;
            }
        }
    }
    return bp;
}
#ifdef MATLAB
mxArray* outputToMxArray (const tagged_array input) 
{
    const int size = input.size - (2*input.offset);
    const int elemtype = sizeof(Compute_float)==sizeof(float)?mxSINGLE_CLASS:mxDOUBLE_CLASS
    mxArray* ret = mxCreateNumericMatrix(size,size,mxSINGLE_CLASS,mxREAL);
    Compute_float* dataptr = (Compute_float*)mxGetPr(ret);
    for (int i=0;i<input.size;i++)
    {
        for (int j=0;j<input.size;j++)
        {
            const Compute_float val =  input.data[(i+input.offset)*input.size + j + input.offset ];
            dataptr[i*size+j]=val;
        }
    }
    return ret;
}
#endif
