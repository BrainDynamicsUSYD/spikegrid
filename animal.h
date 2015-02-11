#include "typedefs.h"
///stores the location of an animal
typedef struct animal
{
    Compute_float X; ///< X-coord of location
    Compute_float Y; ///< Y-coord of location
} animal;

void AnimalEffects(const animal a,Compute_float* gE,const Compute_float timemillis);
void MoveAnimal(animal* a, const Compute_float timemillis);
