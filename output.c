#include <stdio.h>
#include "output.h"
#include "picture.h"
//rescale a float to a unit8 from some minimum and maximum range
//currently no error checking and so might produce UB
uint8_t __attribute__((const)) rescalefloat (const Compute_float in,const Compute_float maxval, const Compute_float minval) //rescale to 0-255
{
    return (uint8_t)((in - minval)/(maxval-minval)*(Compute_float)255.0);
}
Compute_float* taggedarrayTocomputearray(const tagged_array input)
{
    const unsigned int size = input.size - (2*input.offset);
    Compute_float * ret = calloc(sizeof(*ret),size*size);
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            const Compute_float val =  input.data[(i+input.offset)*input.size + j + input.offset ];
            ret[i*size+j]=val;
        }
    }
    return ret;
}
//simple function to convert comp_float 2d array to a bitmap that you can then do something with
bitmap_t* FloattoBitmap(const Compute_float* const input,const unsigned int size,const Compute_float maxval, const Compute_float minval)
{
    bitmap_t* bp = (bitmap_t*)malloc(sizeof(bitmap_t));
    bp->pixels = malloc(sizeof(pixel_t)*size*size);
    bp->width=size;
    bp->height=size;
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            const Compute_float val =input[i*size+j];
            if (val > maxval) 
            {
                bp->pixels[i*size+j].red = 255;
                bp->pixels[i*size+j].green = 255;
                bp->pixels[i*size+j].blue = 255;
            }
            else
            {
                bp->pixels[i*size+j].red = rescalefloat(val,maxval,minval);
                bp->pixels[i*size+j].green = 0;
                bp->pixels[i*size+j].blue = 0;
            }
        }
    }
    return bp;
}




#ifdef MATLAB
//When using matlab, we want to be able to output just about any array of stuff.  This function does the work
mxArray* outputToMxArray (const tagged_array input) 
{
    const unsigned int size = input.size - (2*input.offset);
    const int elemtype = sizeof(Compute_float)==sizeof(float)?mxSINGLE_CLASS:mxDOUBLE_CLASS; //We don't support long double yet
    mxArray* ret = mxCreateNumericMatrix((int)size,(int)size,elemtype,mxREAL); //matlab has signed ints for array sizes - really?
    Compute_float* dataptr =  (Compute_float*)mxGetPr(ret);
    Compute_float* actualdata = taggedarrayTocomputearray(input);
    memcpy(datptr,actualdata,sizeof(Compute_float)*size*size);
    return ret;
}
#endif
int printcount=0;
char fnamebuffer[30];
void OutputToPng(const tagged_array input,const Compute_float minval,const Compute_float maxval)
{
    const unsigned int size = input.size - (2*input.offset);
    Compute_float* actualdata=taggedarrayTocomputearray(input);
    bitmap_t* b = FloattoBitmap(actualdata,size,minval,maxval);
    sprintf(fnamebuffer,"pics/%i.png",printcount);
    printcount++;
    save_png_to_file(b,fnamebuffer);
    free(b->pixels);
    free(b);
    free(actualdata);
    
}

void makemovie(const layer_t l,const unsigned int t)
{
    if (l.P->Movie.MakeMovie==ON && t % l.P->Movie.Delay)
    {
        OutputToPng(l.P->Movie.output.data,l.P->Movie.output.minval,l.P->Movie.output.maxval);
    }
}
