#include "mymath.h"
#include "animal.h"
#include "sizes.h"
#include "localstim.h"

const Compute_float period = 460.0;
void AnimalEffects(const animal a,Compute_float* gE,const Compute_float timemillis)
{
    ApplyLocalBoost(gE,(int)a.X,(int)a.Y);
    const Compute_float timeperiodfrac = fmod(timemillis,period) / period;
    if (timemillis<period) {return;} //avoid first iteration effects
    if (timeperiodfrac < 0.01) {gE[Conductance_index(0,0)]+=30;}
    else if (fabs(timeperiodfrac-0.5)<0.005) {gE[Conductance_index(0,1)]+=30;}
}
void MoveAnimal(animal* a, const Compute_float timemillis)
{
    const Compute_float rad = (Compute_float)grid_size/4.0; //use 3 here to avoid any issues with the boundary
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
