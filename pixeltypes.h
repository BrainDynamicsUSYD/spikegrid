/// \file
#ifndef PIXELTYPES
#define PIXELTYPES
#include <stdint.h>
#include <stdlib.h> //size_t
///structure to represent a single pixel.
typedef struct {
    uint8_t red;    ///<red value
    uint8_t green;  ///<green value
    uint8_t blue;   ///<blue value
} pixel_t;
///structure for a bitmap
typedef struct  {
    pixel_t *pixels;    ///<the actual pixels
    size_t width;       ///<width of image
    size_t height;      ///<height of image
} bitmap_t;
#endif
