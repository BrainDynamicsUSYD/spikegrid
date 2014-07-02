#include "layer.h"
#include "output.h"
#include <stdlib.h>
void FreeIfNotNull(void* v)
{
    if (v != NULL) {free(v);}
}
void CleanupLayer(layer* l)
{
    FreeIfNotNull(l->STDP_connections);
    FreeIfNotNull(l->voltages);
    FreeIfNotNull(l->voltages_out);
    FreeIfNotNull(l->recoverys);
    FreeIfNotNull(l->recoverys_out);
    for (unsigned int i = 0;i<l->spikes.count;i++)
    {
        free(l->spikes.data[i]);
    }
    FreeIfNotNull(l->spikes.data);

}
void CleanupModel(model* m)
{ 
    CleanupLayer(&m->layer1);
    if (m->NoLayers==DUALLAYER)
    {
        CleanupLayer(&m->layer2);
    }
    CleanupOutput();
}
