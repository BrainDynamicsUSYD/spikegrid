#include <stdio.h>
#include "cson/include/wh/cson/cson.h"
#include "model.h"
void SaveArray(const Compute_float* arr,const unsigned int count,const char* const name,cson_object* json)
{
    cson_value* av = cson_value_new_array();
    cson_array* a  = cson_value_get_array(av);
    cson_object_set(json,name,av);
    for (unsigned int i=0;i<count;i++)
    {
        cson_value* v = cson_value_new_double(arr[i]);
        cson_array_append(a,v);
        cson_value_free(v);
    }
}
void SaveArray_int16 (const int16_t* arr,const unsigned int count,const char* const name,cson_object* json)
{
    cson_value* av = cson_value_new_array();
    cson_array* a  = cson_value_get_array(av);
    cson_object_set(json,name,av);
    for (unsigned int i=0;i<count;i++)
    {
        cson_value* v = cson_value_new_integer(arr[i]);
        cson_array_append(a,v);
        cson_value_free(v);
    }
}
void lagstorage_to_cson(const lagstorage* L,cson_object* json)
{
    cson_object* l = cson_new_object();

}
void layer_to_cson(const layer* L, cson_object* json)
{
#define LAYER_SAVEARRAY(arg,size) SaveArray(l->arg,size*size,#size,l)
    cson_object* l = cson_new_object();
    LAYER_SAVEARRAY(connections,couple_array_size);
    LAYER_SAVEARRAY(voltages,grid_size);
    LAYER_SAVEARRAY(voltages_out,grid_size);
    LAYER_SAVEARRAY(recoverys,grid_size);
    LAYER_SAVEARRAY(recoverys_out,grid_size);
    LAYER_SAVEARRAY(recoverys_out,grid_size);
    LAYER_SAVEARRAY(Extimecourse,L->cap);
    LAYER_SAVEARRAY(Intimecourse,L->cap);
    LAYER_SAVEARRAY(Mytimecourse,L->cap);
    //do something with rcinfo - note that the scary data can always be regenerated
    //do something with lagstorage
    //do something with STDP_data
    //do something with parameters
    //do something with STD data
    //lay_os inhib
    //cap
}
SaveModel(const model* const m)
{
    cson_value* jsonv = cson_value_new_object();
    cson_object* json = cson_value_get_object(jsonv);
    cson_object_set(json,"timesteps",cson_value_new_integer(m->timesteps));
    cson_object_set(json,"NoLayers", cson_value_new_integer(m->NoLayers));
    //now do layers
    layer_to_cson(&m->layer1,json);
    layer_to_cson(&m->layer2,json);
    SaveArray(m->gE,conductance_array_size*conductance_array_size,"gE",json);
    SaveArray(m->gI,conductance_array_size*conductance_array_size,"gI",json);
    //do something with the animal object here
    cson_output_FILE(jsonv,stdout,NULL);
}
