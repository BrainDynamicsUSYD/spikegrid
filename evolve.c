/// \file
#include <string.h> //memset
#include <stdlib.h> //random
#include "theta.h"
#include "output.h"
#include "STDP.h"
#include "evolvegen.h"
#include "STD.h"
#include "mymath.h"
#include "paramheader.h"
#include "coupling.h"
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
///Add conductances in the presence of STDP
void evolvept_STDP  (const int x,const  int y,const Compute_float* const __restrict connections_STDP,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI)
{
    if (Features.STDP == OFF) {return;}
    evolvept(x,y,&(connections_STDP[(x*grid_size +y)*couple_array_size*couple_array_size]),Estrmod,Istrmod,gE,gI);
}
///Adds the effect of the spikes that have fired in the past to the gE and gI arrays as appropriate
/// currently, single layer doesn't work (correctly)
void AddSpikes(layer L, Compute_float* __restrict__ gE, Compute_float* __restrict__ gI,const unsigned int time)
{
 //   const int Eon = L.Extimecourse!=NULL; //does this layer have excitation
    const int Ion = L.Intimecourse!=NULL; //and inhibition
    const decay_parameters D = L.P->couple.Layer_parameters.dual.synapse;
    for (int y=0;y<grid_size;y++)
    {
        for (int x=0;x<grid_size;x++)
        {
            Compute_float str = Zero;
            const int idx = x*grid_size + y;
            int idx2 = 0;
            while (L.firinglags.lags[idx*L.firinglags.lagsperpoint+idx2] != -1)
            {
                Compute_float this_str = Synapse_timecourse(D,L.firinglags.lags[idx*L.firinglags.lagsperpoint + idx2] * Features.Timestep);
                if (Features.STD == ON)
                {
                    this_str = this_str * STD_str(L.P->STD,x,y,time,L.firinglags.lags[idx*L.firinglags.lagsperpoint + idx2],L.std);
                }
                str += this_str;
                idx2++;
            }
            if (Ion) {str = (-str);} //invert strength for inhib conns.
            if (idx2 > 0) //only fire if we had a spike.
            {
                evolvept_duallayer(x,y,L.connections,str,(Ion?gI:gE));
                if (Features.STDP==ON)
                {
                    evolvept_duallayer_STDP(x,y,L.connections,L.STDP_data->connections,str,(Ion?gI:gE));
                }
            }
            if (Features.Random_connections == ON)
            {
                const int randbase = (x*grid_size+y)*L.P->random.numberper;
                for (int i=0;i<L.P->random.numberper;i++)
                {
                    const randomconnection rc = L.randconns[randbase+i];
                    const int condindex = (rc.destination.x + couplerange) * conductance_array_size + rc.destination.y + couplerange;
                    if (Ion) {gI[condindex] += str * rc.strength;}
                    else  {gE[condindex] += str * rc.strength;}
                }
            }
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
            gE[(grid_size+i)*conductance_array_size + j] += gE[i*conductance_array_size+j]; //add to bottom
            gE[(i+couplerange)*conductance_array_size+j] += gE[(grid_size+couplerange+i)*conductance_array_size+j];//add to top
            gI[(grid_size+i)*conductance_array_size + j] += gI[i*conductance_array_size+j]; //add to bottom
            gI[(i+couplerange)*conductance_array_size+j] += gI[(grid_size+couplerange+i)*conductance_array_size+j];//add to top
		}
	}
    //left + right boundary condition fix
    for (int i=couplerange;i<couplerange+grid_size;i++)
	{
        for (int j=0;j<couplerange;j++)
		{
             gE[i*conductance_array_size +grid_size+j ]  += gE[i*conductance_array_size+j];//left
             gE[i*conductance_array_size +couplerange+j] += gE [i*conductance_array_size + grid_size+couplerange+j];//right
             gI[i*conductance_array_size +grid_size+j ]  += gI[i*conductance_array_size+j];//left
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

void modifyLags(lagstorage* L,int baseidx)
{
    //first - increment the firing lags.
    int idx = 0;
    while (L->lags[baseidx+idx] != -1)
    {
        L->lags[baseidx+idx]++;
        idx++;
    }
    if (L->lags[baseidx] == L->cap )//if first entry is at cap - remove and shuffle everything down
    {
        int idx2 = 0;
        while (L->lags[baseidx+idx2] != -1) //move everthing down
        {
            L->lags[baseidx+idx2] = L->lags[baseidx+idx2+1];
            idx2++;
        }
    }
}

void AddnewSpike(lagstorage* L,const int baseidx)
{
    //find the empty idx
    int idx = 0;
    while (L->lags[baseidx + idx] != -1)
    {
        idx++;
    }
    //and set it to 1 - this makes things work
    L->lags[baseidx + idx]=1;
    //and set the next one to -1 to mark the end of the array
    L->lags[baseidx + idx+1]= -1;
}


///Store current firing spikes also apply random spikes
void StoreFiring(layer* L)
{
    const int step = (int)L->P->skip;
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            const int test = x % step ==0 && y % step ==0;
            if ((test && step > 0) || ((!test) && step<0))
            {
                const int baseidx=(x*grid_size+y)*L->firinglags.lagsperpoint;
                modifyLags(&L->firinglags,baseidx);
                if (Features.STDP==ON) {modifyLags(&L->STDP_data->lags,(x*grid_size+y)*L->STDP_data->lags.lagsperpoint);}
                //now - add in new spikes
                if (L->voltages_out[x*grid_size + y]  >= L->P->potential.Vpk)
                {
                    if (Features.Recovery==ON) //reset recovery if needed
                    {
                        L->recoverys_out[x*grid_size+y]+=L->P->recovery.Wrt;
                    }
                AddnewSpike(&L->firinglags,baseidx);
                if (Features.STDP==ON) {AddnewSpike(&L->STDP_data->lags,(x*grid_size+y)*L->STDP_data->lags.lagsperpoint);}
                }//add random spikes
                else if (((Compute_float)random())/((Compute_float)RAND_MAX) <
                        (L->P->potential.rate*((Compute_float)0.001)*Features.Timestep))
                {
                    L->voltages_out[x*grid_size+y]=L->P->potential.Vpk+(Compute_float)0.1;//make sure it fires - the neuron will actually fire next timestep
                }
            }
            else
            {
                    L->voltages_out[x*grid_size+y]=Zero; //skipped neurons set to 0
            }
        }
    }
}
///Cleans up voltages for neurons that are in the refractory state
void ResetVoltages(Compute_float* const __restrict Vout,const couple_parameters C,const lagstorage l,const conductance_parameters CP)
{
    const int trefrac_in_ts =(int) ((Compute_float)C.tref / Features.Timestep);
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            int idx=0;
            while (l.lags[(x*grid_size+y)*l.lagsperpoint+ idx] != -1)
            {
                idx++;
            } //take the last entry - then check if in refrac period and set voltages
            if (idx >0 && l.lags[(x*grid_size+y)*l.lagsperpoint + idx-1] <= trefrac_in_ts)
            {
                Vout[x*grid_size + y] = CP.Vrt;
            }
        }
    }
}
void tidylayer (layer* l,const unsigned int time,const Compute_float timemillis,const Compute_float* const gE,const Compute_float* const gI)
{
    // without recovery variable
    if (Features.Recovery==OFF)
    {
        CalcVoltages(l->voltages,gE,gI,l->P->potential,l->voltages_out);
        ResetVoltages(l->voltages_out,l->P->couple,l->firinglags,l->P->potential);
    }
    // with recovery variable (note no support for theta - no idea if they work together)
    else
    {
        CalcRecoverys(l->voltages,l->recoverys,gE,gI,l->P->potential,l->P->recovery,l->voltages_out,l->recoverys_out);
    }
    StoreFiring(l);
    dooutput(l->P->output,time);
    if (Features.Theta==ON)
    {
        dotheta(l->voltages_out,l->P->theta,timemillis);
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
    tidylayer(&m->layer1,time,timemillis,m->gE,m->gI);
    if (m->NoLayers==DUALLAYER){tidylayer(&m->layer2,time,timemillis,m->gE,m->gI);}
    if (Features.STDP==ON)
    {
        DoSTDP(m->layer1.connections,m->layer2.connections,m->layer1.STDP_data,m->layer1.P->STDP, m->layer2.STDP_data,m->layer2.P->STDP,m->layer1.randconns,&m->layer1.P->random);
        DoSTDP(m->layer2.connections,m->layer1.connections,m->layer2.STDP_data,m->layer2.P->STDP, m->layer1.STDP_data,m->layer1.P->STDP,m->layer2.randconns,&m->layer2.P->random);
    }
}
