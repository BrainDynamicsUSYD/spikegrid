/// \file
#include <string.h> //memset
#include <stdlib.h> //random
#include "theta.h"
#include "output.h"
#include "STDP.h"

///add conductance from a firing neuron to the gE and gI arrays (used in single layer model)
void evolvept (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI)
{
    for (int i = 0; i < couple_array_size;i++)
    {
        const int outoff = (x + i)*conductance_array_size +y;//as gE and gI are larger than the neuron grid size, don't have to worry about wrapping
        for (int j = 0 ; j<couple_array_size;j++) 
        {
            const int coupleidx = i*couple_array_size + j;
            if (connections[coupleidx] > 0) 
            {
                gE[outoff+j] += connections[coupleidx]*Estrmod;
            }
            else
            {
                gI[outoff+j] += -(connections[coupleidx]*Istrmod);
            }
        }
    } 
}

//when STDP is turned off, gcc will warn about this function needing const. It is wrong
void evolvept_STDP  (const int x,const  int y,const Compute_float* const __restrict connections_STDP,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI)
{
    if (Features.STDP == OFF) {return;}
    evolvept(x,y,&(connections_STDP[(x*grid_size +y)*couple_array_size*couple_array_size]),Estrmod,Istrmod,gE,gI);
}
///add conductance from a firing neuron to either gE or gI as appropriate (used in dual layer model)
void evolvept_duallayer (const int x,const  int y,const Compute_float* const __restrict connections,const Compute_float strmod, Compute_float* __restrict condmat)
{
    for (int i = 0; i < couple_array_size;i++)
    {
        const int outoff = (x + i)*conductance_array_size +y;//as gE and gI are larger than the neuron grid size, don't have to worry about wrapping
        for (int j = 0 ; j<couple_array_size;j++) 
        {
            const int coupleidx = i*couple_array_size + j;
            condmat[outoff+j] += connections[coupleidx]*strmod;
        }
    } 
//    Need to find a way to do STDP here
}
///Adds the effect of the spikes that have fired in the past to the gE and gI arrays as appropriate
void AddSpikes(layer L, Compute_float* __restrict__ gE, Compute_float* __restrict__ gI,const unsigned int time)
{
    for (unsigned int i=1;i<L.spikes.count;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {
        const coords* const fire_with_this_lag = ringbuffer_getoffset(&L.spikes,(int)i);
        const int Eon = L.Extimecourse!=NULL; //does this layer have excitation
        const int Ion = L.Intimecourse!=NULL; //and inhibition
        const Compute_float Estr =Eon? L.Extimecourse[i]:Zero;
        const Compute_float Istr =Ion? L.Intimecourse[i]:Zero; 
        int idx=0; //iterate through all neurons firing with this lag
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; //add conductances
            Compute_float strmod=One;
            if (Features.STD == ON)
            {
                strmod=STD_str(L.P->STD,c.x,c.y,time,i,(L.std));
            }
            if (Eon && Ion) 
            {
                evolvept(c.x,c.y,L.connections,Estr*strmod,Istr*strmod,gE,gI);
                evolvept_STDP(c.x,c.y,L.STDP_connections,Estr*strmod,Istr*strmod,gE,gI);
            }
            else {evolvept_duallayer(c.x,c.y,L.connections,(Ion?Istr*-1:Estr)*strmod,(Ion?gI:gE));} //TODO: STDP not implemented in dual-layer
            idx++;
        }
    }
}

///This function adds in the overlapping bits back into the original matrix.  It is slightly opaque but using pictures you can convince yourself that it works
///To keep the evolvept code simple we use an array like this:
///~~~~
///    +–––––––––––––––––+          
///    |Extra for overlap|          
///    |  +––––––––––+   |          
///    |  |Actual    |   |          
///    |  |matrix    |   |          
///    |  +––––––––––+   |          
///    +–––––––––––––––––+ 
///~~~~
void fixboundary(Compute_float* __restrict gE, Compute_float* __restrict gI)
{   //theoretically the two sets of loops could be combined but that would be incredibly confusing
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

///rhs_func used when integrating the neurons forward through time.  The actual integration is done using the midpoint method
Compute_float __attribute__((const,pure)) rhs_func  (const Compute_float V,const Compute_float ge,const Compute_float gi,const conductance_parameters p)
{
    switch (p.type.type) 
    {   
        case LIF:
            return -(p.glk*(V-p.Vlk) + ge*(V-p.Vex) + gi*(V-p.Vin));
        case QIF:
            return -(p.glk*(V-p.Vlk)*(p.type.extra.QIF.Vth-V) + ge*(V-p.Vex) + gi*(V-p.Vin));
        case EIF: 
            return -(p.glk*(V-p.Vlk) - p.glk*p.type.extra.EIF.Dpk*exp((V-p.type.extra.EIF.Vth)/p.type.extra.EIF.Dpk) + ge*(V-p.Vex) + gi*(V-p.Vin));
        default: return One; //avoid -Wreturn-type error which is probably wrong anyway
    }
}

///Uses precalculated gE and gI to integrate the voltages forward through time.
void CalcVoltages(const Compute_float* const __restrict__ Vinput,
        const Compute_float* const __restrict__ gE, 
        const Compute_float* const __restrict__ gI,
        const conductance_parameters C, 
        Compute_float* const __restrict__ Vout)
{
    for (int x=0;x<grid_size;x++) 
    {
        for (int y=0;y<grid_size;y++)
        { //step all neurons through time 
            const int idx = (x+couplerange)*conductance_array_size + y + couplerange; //index for gE/gI
            const int idx2=  x*grid_size+y;
            // apply Euler method
            const Compute_float rhs = rhs_func(Vinput[idx2],gE[idx],gI[idx],C);
            Vout[idx2]=Vinput[idx2]+Features.Timestep*rhs;
        }
    }
}
///Uses precalculated gE and gI to integrate the voltages and recoverys forward through time. This uses the Euler method
void CalcRecoverys(const Compute_float* const __restrict__ Vinput, 
        const Compute_float* const __restrict__ Winput, 
        const Compute_float* const __restrict__ gE, 
        const Compute_float* const __restrict__ gI,
        const conductance_parameters C, 
        const recovery_parameters R, 
        Compute_float* const __restrict__ Vout, 
        Compute_float* const __restrict__ Wout)
{    // Adaptive quadratic integrate-and-fire
    for (int x=0;x<grid_size;x++) 
    {
        for (int y=0;y<grid_size;y++)
        {   //step all neurons through time - use Euler method
            const int idx = (x+couplerange)*conductance_array_size + y + couplerange; //index for gE/gI
            const int idx2 = x*grid_size+y;       //index for voltage/recovery
            const Compute_float rhsV=rhs_func(Vinput[idx2],gE[idx],gI[idx],C)-Winput[idx2];
            const Compute_float rhsW=R.Wcv*(R.Wir*(Vinput[idx2]-R.Wrt) - Winput[idx2]);
            Vout[idx2] = Vinput[idx2] + Features.Timestep*rhsV;
            Wout[idx2] = Winput[idx2] + Features.Timestep*rhsW;
        }
    }
}

///Store current firing spikes also apply random spikes
void StoreFiring(layer* L)
{ 
    coords* current_firestore = L->spikes.data[L->spikes.curidx];//get the thing for currently firing neurons. Normally 1 is NOT added to curidx
    int this_fcount=0;
    const int step =  L->P->skip;
    for (int16_t x=0;x<grid_size;x+=(int16_t)1)
    {
        for (int16_t y=0;y<grid_size;y+=(int16_t)1)
        {
            if (x % step ==0 && y% step ==0)
            {
                if (L->voltages_out[x*grid_size + y]  >= L->P->potential.Vpk)
                {
                    current_firestore[this_fcount] =(coords){.x=x,.y=y};
                    // Reset recovery variable if applicable
                    if (Features.Recovery==ON)
                    {
                        L->recoverys_out[x*grid_size+y]+=L->P->recovery.Wrt;
                    }
                    this_fcount++;
                }
                else if (((Compute_float)random())/((Compute_float)RAND_MAX) < 
                        (L->P->potential.rate*((Compute_float)0.001)*Features.Timestep))
                {
                    L->voltages_out[x*grid_size+y]=L->P->potential.Vpk+(Compute_float)0.1;//make sure it fires
                }
            }
            else {L->voltages_out[x*grid_size+y]=Zero;}
        }
    }
    current_firestore[this_fcount].x=-1;
}
///Cleans up voltages for neurons that are in the refractory state
void ResetVoltages(Compute_float* const __restrict Vout,const couple_parameters C,const ringbuffer* const spikes,const conductance_parameters CP)
{
    const int trefrac_in_ts =(int) ((Compute_float)C.tref / Features.Timestep);
    for (int i=1;i<=trefrac_in_ts;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {   //put refractory neurons at reset potential
        const coords* const fire_with_this_lag = ringbuffer_getoffset(spikes,i);
        int idx=0;
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; 
            Vout[c.x*grid_size+c.y]=CP.Vrt;
            idx++;
        }
    }

}
///Steps a model through 1 timestep - quite high-level function
void step1(model* m,const unsigned int time)
{
    const Compute_float timemillis = ((Compute_float)time) * Features.Timestep ;
    memset(m->gE,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size); //zero the gE/gI matrices so they can be reused for this timestep
    memset(m->gI,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size);
    // Add spiking input to the conductances
    AddSpikes(m->layer1,m->gE,m->gI,time);
    if (m->NoLayers==DUALLAYER) {AddSpikes(m->layer2,m->gE,m->gI,time);}
    fixboundary(m->gE,m->gI);
    // Add constant input to the conductances
    for (int i = 0;i < conductance_array_size*conductance_array_size;i++)
    {
        m->gE[i] += Extinput.gE0;
        m->gI[i] += Extinput.gI0;
    } 
    //from this point the GE and GI are actually fixed - as a result there is no more layer interaction - so do things sequentially to each layer
    // without recovery variable
    if (Features.Recovery==OFF) 
    {
        CalcVoltages(m->layer1.voltages,m->gE,m->gI,m->layer1.P->potential,m->layer1.voltages_out);
        ResetVoltages(m->layer1.voltages_out,m->layer1.P->couple,&m->layer1.spikes,m->layer1.P->potential);
        if(m->NoLayers==DUALLAYER)
        {
            CalcVoltages(m->layer2.voltages,m->gE,m->gI,m->layer2.P->potential,m->layer2.voltages_out);
            ResetVoltages(m->layer2.voltages_out,m->layer2.P->couple,&m->layer2.spikes,m->layer2.P->potential);
        }
    }
    // with recovery variable (note no support for theta - no idea if they work together)
    else 
    {
        CalcRecoverys(m->layer1.voltages,m->layer1.recoverys,m->gE,m->gI,m->layer1.P->potential,m->layer1.P->recovery,m->layer1.voltages_out,m->layer1.recoverys_out);
        if (m->NoLayers==DUALLAYER) {CalcRecoverys(m->layer2.voltages,m->layer2.recoverys,m->gE,m->gI,m->layer2.P->potential,m->layer2.P->recovery,m->layer2.voltages_out,m->layer2.recoverys_out);}
    }
    StoreFiring(&(m->layer1));
    dooutput(m->layer1.P->output,time);
    if (m->NoLayers==DUALLAYER)
    {
        StoreFiring(&(m->layer2));
        dooutput(m->layer2.P->output,time);
    }
    if (Features.Theta==ON)
    {
        dotheta(m->layer1.voltages_out,m->layer1.P->theta,timemillis);
        if (m->NoLayers==DUALLAYER) {dotheta(m->layer2.voltages_out,m->layer2.P->theta,timemillis);}
    }
    if (Features.STDP==ON)
    {
        doSTDP(m->layer1.STDP_connections,&m->layer1.spikes,m->layer1.connections,m->layer1.P->STDP);
        if (m->NoLayers==DUALLAYER) {doSTDP(m->layer2.STDP_connections,&m->layer2.spikes,m->layer2.connections,m->layer2.P->STDP);}
    }
}
