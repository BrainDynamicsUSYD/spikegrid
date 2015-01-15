#include "typedefs.h"

typedef struct animal
{
    Compute_float X;
    Compute_float Y;
} animal;

void AnimalEffects(const animal a,Compute_float* gE,const unsigned int time);
void MoveAnimal(animal* a, const Compute_float timemillis);
