#include "parameters.h"
typedef struct STD_data
{   //some parts of this should be const - but oh well
    int ftimes[grid_size*grid_size];
    float U[grid_size*grid_size];
    float R[grid_size*grid_size];
} STD_data;

void STD_init();
STD_data STD;