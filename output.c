/// \file
#include <string.h>
#include <unistd.h>
#include "output.h"
#include "picture.h"
#define output_count  11
output_s* Outputtable;
FILE* outfiles[output_count];
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
///convert a tagged array to a PNG.  Paths are auto-calculated
void outputToPng(const tagged_array input,const int idx,const unsigned int count)
{
    char fnamebuffer[30];
    const unsigned int size = input.size - (2*input.offset);
    Compute_float* actualdata=taggedarrayTocomputearray(input);
    bitmap_t* b = FloattoBitmap(actualdata,size,input.minval,input.maxval);
    sprintf(fnamebuffer,"%s/%i-%i.png",outdir,idx,count);
    save_png_to_file(b,fnamebuffer);
    free(b->pixels);
    free(b);
    free(actualdata);
    
}
///TODO: Need to get a better way of detecting when rendering has finished
void outputToConsole(const tagged_array input)
{
    if (!isatty(fileno(stdout))) {return;} //if we are not outputting to a terminal - dont show pictures on console
    char* buf = malloc(sizeof(char)*1000*1000);//should be plenty
    char* upto = buf;
    const unsigned int size = input.size - (2*input.offset);
    Compute_float* actualdata=taggedarrayTocomputearray(input);
    bitmap_t* b = FloattoBitmap(actualdata,size,input.minval,input.maxval);
    printf("\x1b[2J");
    for (unsigned int i=0;i<size;i++)
    {
        for (unsigned int j=0;j<size;j++)
        {
            const pixel_t pix = b->pixels[i*size+j];
            int r = sprintf(upto,"\x1b[48;2;%i;%i;%im ",pix.red,pix.green,pix.blue);
            upto += (r);
        }
        int q = sprintf(upto,"\x1b[0m\n");
        upto += q;
    }
    puts(buf);
    usleep(50000);
    upto=buf;
    free(buf);

}
void outputToText(const output_s input,const int idx)
{
    if (outfiles[idx]==NULL) 
    {
        char buf[100];
        sprintf(buf,"%s/%i.txt",outdir,idx);
        outfiles[idx]=fopen(buf,"w");
    }
    switch (input.data_type)
    {
        case FLOAT_DATA:
        {
            Compute_float* const data = taggedarrayTocomputearray(input.data.TA_data);
            const unsigned int size = input.data.TA_data.size - (2*input.data.TA_data.offset);
            for (unsigned int i=0;i<size;i++)
            {
                for (unsigned int j=0;j<size;j++)
                {
                    fprintf(outfiles[idx],"%f,",data[i*size+j]);
                }
                fprintf(outfiles[idx],"\n");
            }
            fflush(outfiles[idx]);//prevents stalling in matlab
            free(data);
            break;
        }
        case RINGBUFFER_DATA:
        {
            const coords* const data = ringbuffer_getoffset(input.data.RB_data,0);
            int i=0;
            while (data[i].x != -1)
            {
                fprintf(outfiles[idx],"%i,%i;",data[i].x,data[i].y);
                i++;
            }
            fprintf(outfiles[idx],"\n");
            fflush(outfiles[idx]);
            break;
        }
        default:
        printf("I don't know how to output this data\n");

    }
}
///High level function to do output
void dooutput(const output_parameters* const m,const unsigned int t)
{
    int i = 0;
    while (m[i].output_method != NO_OUTPUT)
    {
        if (t % m[i].Delay==0)
        {
            switch (m[i].output_method)
            {
                case PICTURE:
                    if (Outputtable[m[i].Output].data_type == FLOAT_DATA) { outputToPng(Outputtable[m[i].Output].data.TA_data,i,t/m[i].Delay);}
                    else {printf("can't output a ringbuffer to a picture\n");}
                    break;
                case TEXT:
                    outputToText(Outputtable[m[i].Output],i);
                    break;
                case CONSOLE:
                    if (Outputtable[m[i].Output].data_type == FLOAT_DATA) { outputToConsole(Outputtable[m[i].Output].data.TA_data);}
                    else {printf("can't output a ringbuffer to the console\n");}
                    break;
                default:
                    printf("unknown output method\n");
            }
        }
        i++;
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
void output_init(const model* const m)
{
    //WHEN YOU ADD SOMETHING - INCREASE OUTPUT_COUNT AT TOP OF FILE;
    output_s* outdata=(output_s[]){ //note - neat feature - missing elements initailized to 0
        //Name          data type                  actual data                size                    offset         minval,maxval
        {"gE",          FLOAT_DATA, .data.TA_data={m->gE,                     conductance_array_size, couplerange,   0,0.05}}, //gE is a 'large' matrix - as it wraps around the edges
        {"gI",          FLOAT_DATA, .data.TA_data={m->gI,                     conductance_array_size, couplerange,   0,2}}, //gI is a 'large' matrix - as it wraps around the edges
        {"Coupling1",   FLOAT_DATA, .data.TA_data={m->layer1.connections,     couple_array_size,      0,             0,100}}, //return the coupling matrix of layer 1 //TODO: fix min and max values
        {"Coupling2",   FLOAT_DATA, .data.TA_data={m->layer2.connections,     couple_array_size,      0,             0,100}}, //return the coupling matrix of layer 2
        {"V1",          FLOAT_DATA, .data.TA_data={m->layer1.voltages_out,    grid_size,              0,             m->layer1.P->potential.Vin,m->layer1.P->potential.Vpk}},
        {"V2",          FLOAT_DATA, .data.TA_data={m->layer2.voltages_out,    grid_size,              0,             m->layer2.P->potential.Vin,m->layer2.P->potential.Vpk}},
        {"Recovery1",   FLOAT_DATA, .data.TA_data={m->layer1.recoverys_out,   grid_size,              0,             0,100}}, //TODO: ask adam for max and min recovery values
        {"Recovery2",   FLOAT_DATA, .data.TA_data={m->layer2.recoverys_out,   grid_size,              0,             0,100}}, //TODO: ask adam for max and min recovery values
        //ringbuffer outputs
        //name          data type        actual data
        {"Firing1",     RINGBUFFER_DATA, .data.RB_data=&m->layer1.spikes}, //take reference as the struct gets modified
        {"Firing2",     RINGBUFFER_DATA, .data.RB_data=&m->layer2.spikes},
        {.name={0}}};         //a marker that we are at the end of the outputabbles list
    output_s* malloced = malloc(sizeof(output_s)*output_count);
    memcpy(malloced,outdata,sizeof(output_s)*output_count);
    Outputtable = malloced;
}
