/// \file
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "STD.h"
#include "paramheader.h" //needed because some of the max/min values are obtained from parameters
#include "STDP.h"
#include "model.h"
#include "output.h"
#include "tagged_array.h"
#include "mymath.h"
#include "out/outputtable.h"
//TODO: This file might work better as C++ (maybe)
///Total number of things to be output - occasionally needs to be incremented
#define output_count   20
#define overlay_count  3
//declare the extern variables from the header
char outdir[100];
output_s* Outputtable;
//outptu holds an open reference to the model - this enables the mini functions to work
const model* modelref; //MASSIVE HACK


int __attribute__((pure)) Trialno()
{
    return (int) (floor(modelref->timesteps * Features.Timestep /modelref->layer1.P->Stim.timeperiod - modelref->layer1.P->Stim.PreconditioningTrials))  ;
}
int __attribute__((pure)) Timestep() {return (int) modelref->timesteps;}
///Set up the outputtables for a given model
///This function should probably move to the C++ code
///@param m the model we are going to output stuff from
void output_init(const model* const m)
{
    modelref = m; //store ref to model.
    CreateOutputtable((output_s){"gE",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->gE,                     conductance_array_size, couplerange,   1,0,2)}); //gE is a 'large' matrix - as it wraps around the edges
    CreateOutputtable((output_s){"gI",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->gI,                     conductance_array_size, couplerange,   1,0,2)});
    CreateOutputtable((output_s){"Coupling1",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.connections,     couple_array_size,      0,             1,-0.5,0.5)});
    CreateOutputtable((output_s){"Coupling2",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.connections,     couple_array_size,      0,             1,-0.5,0.5)});
    CreateOutputtable((output_s){"V1",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.voltages_out,    grid_size,              0,             1,m->layer1.P->potential.Vin,m->layer1.P->potential.Vpk)});
    CreateOutputtable((output_s){"V2",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.voltages_out,    grid_size,              0,             1,m->layer2.P->potential.Vin,m->layer2.P->potential.Vpk)});
    CreateOutputtable((output_s){"Recovery1",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.recoverys_out,   grid_size,              0,             1,0,100)});
    CreateOutputtable((output_s){"Recovery2",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.recoverys_out,   grid_size,              0,             1,0,100)});
    CreateOutputtable((output_s){"STDU1",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer1.std->U:NULL, grid_size, 0,             1,0,1)});
    CreateOutputtable((output_s){"STDR1",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer1.std->R:NULL, grid_size, 0,             1,0,1)});
    CreateOutputtable((output_s){"STDU2",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer2.std->U:NULL, grid_size, 0,             1,0,1)});
    CreateOutputtable((output_s){"STDR2",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STD==ON?m->layer2.std->R:NULL, grid_size, 0,             1,0,1)});
    CreateOutputtable((output_s){"STDP1",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STDP==ON?m->layer1.STDP_data->connections:NULL,grid_size,0,couple_array_size,-0.01,0.01)});
    CreateOutputtable((output_s){"STDP2",       FLOAT_DATA, .data.TA_data=tagged_array_new(Features.STDP==ON?m->layer2.STDP_data->connections:NULL,grid_size,0,couple_array_size,-0.01,0.01)});
    CreateOutputtable((output_s){"Spike1",      SPIKE_DATA, .data.Lag_data=m->layer1.firinglags});
    CreateOutputtable((output_s){"Spike2",      SPIKE_DATA, .data.Lag_data=m->layer2.firinglags});
    CreateOutputtable((output_s){"STDP_map",    FLOAT_DATA,
            .data.TA_data =Features.STDP==ON?tagged_array_new(m->layer2.STDP_data->connections,grid_size,0,1,-0.01,0.01):NULL,
            .Updateable=ON, .UpdateFn=&STDP_mag,
            .function_arg =Features.STDP==ON?tagged_array_new(m->layer2.STDP_data->connections,grid_size,0,1,-0.01,0.01):NULL
        });
    CreateOverlay((overlaytext){"Trialno",Trialno});
    CreateOverlay((overlaytext){"Timestep",Timestep});
}
///Cleans up memory and file handles that are used by the outputtables object
void CleanupOutput()
{
    free(Outputtable);//also cleanup outputtables
    Outputtable=NULL;
}
