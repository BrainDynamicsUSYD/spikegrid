#ifndef ANIMAL
#define ANIMAL
#include "typedefs.h"
///stores the location of an animal
typedef struct Animal
{
    Compute_float X; ///< X-coord of location
    Compute_float Y; ///< Y-coord of location
} Animal;

void AnimalEffects(const Animal a,Compute_float* gE,const Compute_float timemillis);
void MoveAnimal(Animal* a, const Compute_float timemillis);
#endif
