#include "parameters.h"
#include "evolve.h"
#include "helpertypes.h"
#include "STD.h"
#include <string.h> //memset
#include <tgmath.h> //exp
#include <stdio.h> //printf
#include <stdlib.h> //random
//add gE/gI when using STDP - untested
//when STDP is turned off, gcc will warn about this function needing const.  It is wrong
void __attribute__((const)) evolvept_STDP  (const int x,const  int y,const Compute_float* const __restrict connections_STDP,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI)
{
    //ex coupling
    if (Param.features.STDP == OFF) {return;}
    for (int i = 0; i < couple_array_size;i++)
    {
        const int outoff = (x +i )*conductance_array_size +y;//as gE and gI are larger than the neuron grid size, don't have to worry about wrapping
        for (int j = 0 ; j<couple_array_size;j++) 
        {
            const int coupleidx =(x*grid_size +y)*couple_array_size*couple_array_size + i*couple_array_size + j;
            if (connections_STDP[coupleidx] > 0) //add either to gE or gI
            {
                gE[outoff+j] += connections_STDP[coupleidx]*Estrmod;
            }
            else
            {
                gI[outoff+j] += -(connections_STDP[coupleidx]*Istrmod);
            }
        }
    } 
}

//add conductance from a firing neuron the gE and gI arrays
////TODO: add the skip part to skip zero entries as in the threestate code
void evolvept (const int x,const  int y,const Compute_float* const __restrict connections,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI,const Compute_float* STDP_CONNS)
{
    //ex coupling
    
    for (int i = 0; i < couple_array_size;i++)
    {
        const int outoff = (x +i )*conductance_array_size +y;//as gE and gI are larger than the neuron grid size, don't have to worry about wrapping
        for (int j = 0 ; j<couple_array_size;j++) 
        {
            const int coupleidx = i*couple_array_size + j;
            if (connections[coupleidx] > 0) //add either to gE or gI
            {
                gE[outoff+j] += connections[coupleidx]*Estrmod;
            }
            else
            {
                gI[outoff+j] += -(connections[coupleidx]*Istrmod);
            }
        }
    } 
    evolvept_STDP(x,y,STDP_CONNS,Estrmod,Istrmod,gE,gI);
}
void fixboundary(Compute_float* __restrict gE, Compute_float* __restrict gI)
{
    //top + bottom
    for (int i=0;i<couplerange;i++)
	{
        for (int j=0;j<conductance_array_size;j++)
		{
            gE[(grid_size+i)*conductance_array_size + j] +=gE[i*conductance_array_size+j]; //add to bottom
            gE[(i+couplerange)*conductance_array_size+j] += gE[(grid_size+couplerange+i)*conductance_array_size+j];//add to top
            gI[(grid_size+i)*conductance_array_size + j] +=gI[i*conductance_array_size+j]; //add to bottom
            gI[(i+couplerange)*conductance_array_size+j] += gI[(grid_size+couplerange+i)*conductance_array_size+j];//add to top
		}
	}
    //left + right boundary condition fix
    for (int i=couplerange;i<couplerange+grid_size;i++)
	{
        for (int j=0;j<couplerange;j++)
		{
             gE[i*conductance_array_size +grid_size+j ] += gE[i*conductance_array_size+j];//left
             gE[i*conductance_array_size +couplerange+j] += gE [i*conductance_array_size + grid_size+couplerange+j];//right
             gI[i*conductance_array_size +grid_size+j ] += gI[i*conductance_array_size+j];//left
             gI[i*conductance_array_size +couplerange+j] += gI [i*conductance_array_size + grid_size+couplerange+j];//right
		}
	}

}

//these gE/gI values should be static to the function - but sometimes they need to be exposed as read only
Compute_float gE[conductance_array_size*conductance_array_size];
Compute_float gI[conductance_array_size*conductance_array_size];
const volatile Compute_float* const GE = &gE[0]; //volatile is required as the underlying data can change and we are just exposing the read only pointer
const volatile Compute_float* const GI = &gI[0];
//rhs_func used when integrating the neurons forward through time
Compute_float __attribute__((const)) rhs_func  (const Compute_float V,const Compute_float ge,const Compute_float gi,const conductance_parameters p) {return -(p.glk*(V-p.Vlk) + ge*(V-p.Vex) + gi*(V-p.Vin));}
//step the model through time
void step1 (layer_t* layer,const unsigned int time)
{
    coords* current_firestore = layer->spikes.data[layer->spikes.curidx];//get the thing for currently firing neurons
    memset(gE,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size); //zero the gE/gI matrices so they can be reused
    memset(gI,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size);
    for (unsigned int i=1;i<layer->spikes.count;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {
        const coords* const fire_with_this_lag = ringbuffer_getoffset(&layer->spikes,(int)i);
        const int delta =(int)(((Compute_float)i)*Param.time.dt);//small helper constant. TODO: Question - are all these conversions necersarry?
        const Compute_float Estr = layer->Extimecourse[delta];
        const Compute_float Istr = layer->Intimecourse[delta]; 
        int idx=0; //iterate through all neurons firing with this lag
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; //add conductances
            Compute_float strmod=One;
            if (Param.features.STD == ON)
            {
                strmod=STD_str(layer->std.P,c.x,c.y,time,i,&(layer->std));
            }
            evolvept(c.x,c.y,layer->connections,Estr*strmod,Istr*strmod,gE,gI,layer->STDP_connections);
            idx++;
        }
    }
    fixboundary(gE,gI);
    for (int x=0;x<grid_size;x++) 
    {
        for (int y=0;y<grid_size;y++)
        { //step all neurons through time - use midpoint method
            const int idx = (x+couplerange)*conductance_array_size + y + couplerange; //index for gE/gI
            const int idx2=  x*grid_size+y;//index for voltages
            const Compute_float rhs1=rhs_func(layer->voltages[idx2],gE[idx],gI[idx],*(layer->P));
            const Compute_float Vtemp = layer->voltages[idx2] + Half*Param.time.dt*rhs1;
            const Compute_float rhs2=rhs_func(Vtemp,gE[idx],gI[idx],*(layer->P));
            layer->voltages_out[idx2]=layer->voltages[idx2]+Half*Param.time.dt*(rhs1 + rhs2);
        }
    }
    int this_fcount=0;//now find which neurons fired at the current time step
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            if (layer->voltages[x*grid_size + y]  > layer->P->Vth)
            {
                const coords c= {x,y}; //required to keep the compiler happy - can't do an inline constructor
                current_firestore[this_fcount] =c;
                layer->voltages_out[x*grid_size+y]=layer->P->Vrt;
                this_fcount++;
                if (Param.features.Output==ON)
                {
                    printf("%i,%i;",x,y);
                }
            }
            else if (((Compute_float)random())/((Compute_float)RAND_MAX) < (layer->P->rate*((Compute_float)0.001)*Param.time.dt))
            {
                layer->voltages_out[x*grid_size+y]=layer->P->Vth+(Compute_float)0.1;//make sure it fires
            }
        }
    }
    //todo: fix for time scale properly
    for (int i=1;i<=51;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {   //put refractory neurons at reset potential
        const coords* const fire_with_this_lag = ringbuffer_getoffset(&layer->spikes,i);
        int idx=0;
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; 
            layer->voltages_out[c.x*grid_size+c.y]=layer->P->Vrt;
            idx++;
        }
    }
    current_firestore[this_fcount].x=-1;
    if (Param.features.Output==ON &&time % 10 ==0 ) {printf("\n");}

}
