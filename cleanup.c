#include "layer.h"
#include <stdlib.h>
void FreeIfNotNull(void* v)
{
    if (v != NULL) {free(v);}
}
void CleanupModel(model* m)
{ //doesn't do anything yet
}
