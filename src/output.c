/// \file
#include <stdio.h>
#include "paramheader.h" //needed because some of the max/min values are obtained from parameters
#include "model.h"
#include "tagged_array.h"
#include "mymath.h"
#include "out/outputtable.h"
//declare the extern variables from the header
char __attribute__((used)) outdir[100]; //using flto on ubuntu hides this symbol for some reason.  Adding used fixes it
//outptu holds an open reference to the model - this enables the mini functions to work
const model* modelref; //MASSIVE HACK


int __attribute__((pure)) Trialno()
{
    return (int) (floor(modelref->timesteps * Features.Timestep /modelref->layer1.P->Stim.timeperiod - modelref->layer1.P->Stim.PreconditioningTrials))  ;
}
//Used for the simple overlay of timesteps
int __attribute__((pure)) Timestep() {return (int) modelref->timesteps;}
///Set up the outputtables for a given model
///This function should probably move to the C++ code
///@param m the model we are going to output stuff from
void output_init(const model* const m)
{
    modelref = m; //store ref to model.
    //define model based outputs - this could move to model.c
    CreateOutputtable((output_s){"gE",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->cond_matrices->gE,                     conductance_array_size, couplerange,   1,0,2)}); //gE is a 'large' matrix - as it wraps around the edges
    CreateOutputtable((output_s){"gI",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->cond_matrices->gI,                     conductance_array_size, couplerange,   1,0,2)});
    CreateOutputtable((output_s){"Coupling1",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.connections,     couple_array_size,      0,             1,-0.5,0.5)});
    CreateOutputtable((output_s){"Coupling2",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.connections,     couple_array_size,      0,             1,-0.5,0.5)});
    CreateOutputtable((output_s){"V1",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.voltages_out,    grid_size,              0,             1,m->layer1.P->potential.Vin,m->layer1.P->potential.Vpk)});
    CreateOutputtable((output_s){"V2",          FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.voltages_out,    grid_size,              0,             1,m->layer2.P->potential.Vin,m->layer2.P->potential.Vpk)});
    CreateOutputtable((output_s){"Recovery1",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer1.recoverys_out,   grid_size,              0,             1,0,100)});
    CreateOutputtable((output_s){"Recovery2",   FLOAT_DATA, .data.TA_data=tagged_array_new(m->layer2.recoverys_out,   grid_size,              0,             1,0,100)});
    CreateOutputtable((output_s){"Spike1",      SPIKE_DATA, .data.Lag_data=m->layer1.lags});
    CreateOutputtable((output_s){"Spike2",      SPIKE_DATA, .data.Lag_data=m->layer2.lags});
    //note: some outputs defined elsewhere - it seems more convenient
    //Create the overlays - the functions are defined earlier in the file.
    CreateOverlay((overlaytext){"Trialno",Trialno});
    CreateOverlay((overlaytext){"Timestep",Timestep});
}
