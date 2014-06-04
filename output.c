/// \file
#include <string.h>
#include "output.h"
#include "picture.h"

///Extracts the actual information out of a tagged array and converts it to a simple square matrix
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

/// This generates the jet MATLAB colormap. We multiply by 255 because the map distinguishes colors over a range of 0 to 255 integer values inclusive (see http://stackoverflow.com/questions/7706339/grayscale-to-red-green-blue-matlab-jet-color-scale for a full explanation)
/// @param scaledval a float between 0 and 1
/// @param pixel the pixel to modify
void JetCmap(const Compute_float scaledval, pixel_t * pixel)
{
    if (scaledval < 0.125)
    {
        pixel->red = 0;
        pixel->green = 0;
        pixel->blue = (uint8_t)(255*(4*scaledval + 0.5));
    }
    else if (scaledval < 0.375)
    {
        pixel->red = 0;
        pixel->green = (uint8_t)(255*(4*scaledval - 0.5));
        pixel->blue = 255; 
    }
    else if (scaledval < 0.625)
    {
        pixel->red = (uint8_t)(255*(4*scaledval - 1.5));
        pixel->green = 255;
        pixel->blue = (uint8_t)(255*(-4*scaledval + 2.5));
    }
    else if (scaledval < 0.875)
    {
        pixel->red = 255;
        pixel->green = (uint8_t)(255*(-4*scaledval + 3.5));
        pixel->blue = 0;
    }
    else
    {
        pixel->red = (uint8_t)(255*(-4*scaledval + 4.5));
        pixel->green = 0;
        pixel->blue = 0;
    }
}

///simple function to convert comp_float 2d array to a bitmap that you can then do something with (like save)
bitmap_t* FloattoBitmap(const Compute_float* const input,const unsigned int size,const Compute_float minval, const Compute_float maxval)
{
    bitmap_t* bp = (bitmap_t*)malloc(sizeof(bitmap_t));
    bp->pixels = malloc(sizeof(pixel_t)*size*size);
    bp->width=size;
    bp->height=size;
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            Compute_float val =input[i*size+j];
            // Clamp values which fall outside the specified range
            if (val < minval) {val = minval;}
            if (val > maxval) {val = maxval;}
            // Scaled value (between 0 and 1)
            const Compute_float scaledval = (val - minval)/(maxval - minval);
            // Uses Jet colormap as used in MATLAB
            JetCmap(scaledval,&(bp->pixels[i*size+j]));
        }
    }
    return bp;
}

/// number of PNG's outputted.  Used to keep track of the next filename to use
int printcount=0;
///convert a tagged array to a PNG.  Paths are auto-calculated
void OutputToPng(const tagged_array input,const Compute_float minval,const Compute_float maxval)
{
    char fnamebuffer[30];
    const unsigned int size = input.size - (2*input.offset);
    Compute_float* actualdata=taggedarrayTocomputearray(input);
    bitmap_t* b = FloattoBitmap(actualdata,size,minval,maxval);
    sprintf(fnamebuffer,"%s/%i.png",outdir,printcount);
    printcount++;
    save_png_to_file(b,fnamebuffer);
    free(b->pixels);
    free(b);
    free(actualdata);
    
}
void outputToConsole(const tagged_array input, const Compute_float minval,const Compute_float maxval)
{
    char* buf = malloc(sizeof(char)*1000*1000);//should be plenty
    char* upto = buf;
    const unsigned int size = input.size - (2*input.offset);
    Compute_float* actualdata=taggedarrayTocomputearray(input);
    bitmap_t* b = FloattoBitmap(actualdata,size,minval,maxval);
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            const pixel_t pix = b->pixels[i*size+j];
            int r = sprintf(upto,"\x1b[48;2;%i;%i;%im ",pix.red,pix.green,pix.blue);
            upto += (r);
        }
        printf("%s\x1b[0m\n",buf);
        upto=buf;
    }
    free(buf);

}
///High level function to create a series of PNG images which can be then turned into a movie
void makemovie(const layer l,const unsigned int t)
{
    if (l.P->Movie.MakeMovie==ON && t % l.P->Movie.Delay==0)
    {
        OutputToPng(Outputtable[l.P->Movie.Output].data,Outputtable[l.P->Movie.Output].minval,Outputtable[l.P->Movie.Output].maxval);
        outputToConsole(Outputtable[l.P->Movie.Output].data,Outputtable[l.P->Movie.Output].minval,Outputtable[l.P->Movie.Output].maxval);
    }
}

output_s __attribute__((pure)) getOutputByName(const char* const name)
{
    int outidx=0;
    while (strlen(Outputtable[outidx].name) != 0)
    {
        if (!strcmp(Outputtable[outidx].name,name))
        {
            return Outputtable[outidx];
        }
        outidx++;
    } 
    printf("tried to get unknown thing to output\n");
    exit(EXIT_FAILURE);
}
