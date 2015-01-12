#include "mymath.h"
#include "animal.h"
#include "sizes.h"
#include "localstim.h"
void AnimalEffects(const animal a,Compute_float* gE)
{
    ApplyLocalBoost(gE,(int)a.X,(int)a.Y);
}
void MoveAnimal(animal* a, const Compute_float timemillis)
{
    const Compute_float rad = (Compute_float)grid_size/3.0; //use 3 here to avoid any issues with the boundary
    const Compute_float period = 30.0;
    a->X=(Compute_float)(grid_size/2) + rad*sin(timemillis/period);
    a->Y=(Compute_float)(grid_size/2) + rad*cos(timemillis/period);
}
