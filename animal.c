#include <stdlib.h>
#include "mymath.h"
#include "animal.h"
#include "sizes.h"
#include "localstim.h"
void AnimalEffects(const animal a,Compute_float* gE,const unsigned int time)
{
   /* if (time < 5000)*/ {ApplyLocalBoost(gE,(int)a.X,(int)a.Y);}
    if (fabs(a.X - (Compute_float)(grid_size/2)) < 0.1) {gE[Conductance_index(0,0)] += 30;}
}
void MoveAnimal(animal* a, const Compute_float timemillis)
{
    const Compute_float rad = (Compute_float)grid_size/4.0; //use 3 here to avoid any issues with the boundary
    const Compute_float period = 60.0;
    const Compute_float timepperiodfrac = fmod(timemillis,period) / period;
    if (timepperiodfrac<0.5)
    {
        a->X=(Compute_float)(grid_size/2) + rad*sin(timepperiodfrac * 4.0*  M_PI);
        a->Y=(Compute_float)(grid_size/4) + rad*cos(timepperiodfrac * 4.0*  M_PI);
    }
    else
    {
        a->X=(Compute_float)(grid_size/2) + rad*sin(timepperiodfrac * 4.0*   M_PI  );
        a->Y=(Compute_float)(3*grid_size/4) + rad*cos(-timepperiodfrac * 4.0* M_PI - M_PI);
    }
}
