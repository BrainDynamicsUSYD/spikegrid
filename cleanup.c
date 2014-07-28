/// \file
#include <stdlib.h>
#include "output.h"
#include "layer.h"
///Free a pointer if it is not null
///@param v pointer to free
void FreeIfNotNull(void* v)
{
    if (v != NULL) {free(v);}
}
void CleanupRingBuffer(ringbuffer* r)
{
    for (unsigned int i = 0;i<r->count;i++)
    {
        free(r->data[i]);
    }
}
///Free all the memory used by a layer
///@param l layer to free
void CleanupLayer(layer* l)
{
    FreeIfNotNull(l->STDP_connections);
    FreeIfNotNull(l->voltages);
    FreeIfNotNull(l->voltages_out);
    FreeIfNotNull(l->recoverys);
    FreeIfNotNull(l->recoverys_out);
    FreeIfNotNull(l->connections);
    FreeIfNotNull(l->Extimecourse);
    FreeIfNotNull(l->Intimecourse);
    FreeIfNotNull(l->P);
    FreeIfNotNull(l->std);
 //   CleanupRingBuffer(&l->spikes);
   // CleanupRingBuffer(&l->spikes_STDP);
//    FreeIfNotNull(l->spikes.data);

}
///Free all memory used by a model
///@param m model to clean up
void CleanupModel(model* m)
{ 
    CleanupLayer(&m->layer1);
    if (m->NoLayers==DUALLAYER)
    {
        CleanupLayer(&m->layer2);
    }
    CleanupOutput();
    free(m);
}
