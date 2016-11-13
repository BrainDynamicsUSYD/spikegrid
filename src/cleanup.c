/// \file
#include <stdlib.h>
#include "output.h"
#include "model.h"
#include "out/out.h"
#include "lagstorage.h"
///Free a pointer if it is not null
///@param v pointer to free
void FreeIfNotNull(void* v)
{
    if (v != NULL) {free(v);}
}
///Free all the memory used by a layer
///@param l layer to free
void CleanupLayer(layer* l)
{
    //TODO fixme
}
///Free all memory used by a model
///@param m model to clean up
void CleanupModel(model* m)
{
    //TODO - fixme
}
