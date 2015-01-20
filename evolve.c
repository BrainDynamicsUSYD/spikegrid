/// \file
#include <string.h> //memset
#include <stdlib.h> //random
#include <stdio.h>
#include "STDP.h"
#include "model.h"
#include "theta.h"
#include "evolvegen.h"
#include "STD.h"
#include "mymath.h"
#include "paramheader.h"
#include "localstim.h"
#include "animal.h"
#include "randconns.h"
#include "lagstorage.h"
#ifdef ANDROID
    #define APPNAME "myapp"
    #include <android/log.h>
#endif

///Adds the effect of the spikes that have fired in the past to the gE and gI arrays as appropriate
/// currently, single layer doesn't work (correctly)
void AddSpikes(layer L, Compute_float* __restrict__ gE, Compute_float* __restrict__ gI,const unsigned int time)
{
    for (int y=0;y<grid_size;y++)
    {
        for (int x=0;x<grid_size;x++)
        {
            Compute_float str = Zero;
            const int idx = x*grid_size + y;
            int numfirings = 0;
            while (L.firinglags->lags[idx*L.firinglags->lagsperpoint+numfirings] != -1)
            {
                Compute_float this_str =L.Mytimecourse[L.firinglags->lags[idx*L.firinglags->lagsperpoint + numfirings]];
                if (Features.STD == ON)
                {
                    this_str = this_str * STD_str(L.P->STD,x,y,time,L.firinglags->lags[idx*L.firinglags->lagsperpoint + numfirings],L.std);
                }
                str += this_str;
                numfirings++;
            }
            if (L.Layer_is_inhibitory) {str = (-str);} //invert strength for inhib conns.
            if (numfirings > 0) //only fire if we had a spike.
            {
                if (Features.STDP==OFF)
                {
                    evolvept_duallayer(x,y,L.connections,str,(L.Layer_is_inhibitory?gI:gE)); //side note evolvegen doesn't currently work with singlelayer - should probably fix
                }
                else
                {
                    evolvept_duallayer_STDP(x,y,L.connections,L.STDP_data->connections,str,(L.Layer_is_inhibitory?gI:gE));
                }
            }
            if (Features.Random_connections == ON && !L.Layer_is_inhibitory )
            {
                unsigned int norand;
                const randomconnection* rcs = GetRandomConnsLeaving(x,y,*L.rcinfo,&norand);
                for (unsigned int i=0;i<norand;i++)
                {
                    const int condindex = Conductance_index(rcs[i].destination.x,rcs[i].destination.y);
                    if (L.Layer_is_inhibitory) {gI[condindex] += str * (rcs[i].strength + rcs[i].stdp_strength);}
                    else                       {gE[condindex] += str * (rcs[i].strength + rcs[i].stdp_strength);}
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
///Uses eulers method
void CalcVoltages(const Compute_float* const __restrict__ Vinput,
        const Compute_float* const __restrict__ gE,
        const Compute_float* const __restrict__ gI,
        const conductance_parameters C,
        Compute_float* const __restrict__ Vout)
{
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            const int idx =Conductance_index(x,y);
            const int idx2=  x*grid_size+y;
            const Compute_float rhs = rhs_func(Vinput[idx2],gE[idx],gI[idx],C);
            Vout[idx2]=Vinput[idx2]+Features.Timestep*rhs;
        }
    }
}
///Uses precalculated gE and gI to integrate the voltages and recoverys forward through time. This uses the Euler method
//this should probably take a struct as input - way too many arguments
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
            const int idx = Conductance_index(x,y);
            const int idx2 = x*grid_size+y;       //index for voltage/recovery
            const Compute_float rhsV=rhs_func(Vinput[idx2],gE[idx],gI[idx],C)-Winput[idx2];
            const Compute_float rhsW=R.Wcv*(R.Wir*(Vinput[idx2]-C.Vlk) - Winput[idx2]);
            Vout[idx2] = Vinput[idx2] + Features.Timestep*rhsV;
            Wout[idx2] = Winput[idx2] + Features.Timestep*rhsW;
        }
    }
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
            if ((test && step > 0) || ((!test) && step<0)) //check if this is an active neuron
            {
                const int baseidx=LagIdx(x,y,L->firinglags); 
                modifyLags(L->firinglags,baseidx);
                if (Features.STDP==ON) {modifyLags(L->STDP_data->lags,LagIdx(x,y,L->STDP_data->lags));}
                //now - add in new spikes
                if (L->voltages_out[x*grid_size + y]  >= L->P->potential.Vpk)
                {
                    if (Features.Recovery==ON) //reset recovery if needed
                    {
                        L->voltages_out[x*grid_size+y]=L->P->potential.Vrt;                    //does voltage also need to be reset like this?
                        L->recoverys_out[x*grid_size+y]+=L->P->recovery.Wrt;
                    }
                    AddnewSpike(L->firinglags,baseidx);
                    if (Features.STDP==ON) {AddnewSpike(L->STDP_data->lags,LagIdx(x,y,L->STDP_data->lags));}
                }//add random spikes
                else if (((Compute_float)(random()))/((Compute_float)RAND_MAX) <
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
void ResetVoltages(Compute_float* const __restrict Vout,const couple_parameters C,const lagstorage* const  l,const conductance_parameters CP)
{
    const int trefrac_in_ts =(int) ((Compute_float)C.tref / Features.Timestep);
    for (int i=0;i<grid_size*grid_size;i++)
    {
        int baseidx = i*l->lagsperpoint;
        if (CurrentShortestLag(l,baseidx) <= trefrac_in_ts)
        {
            Vout[i] = CP.Vrt;
        }
    }
}
#include "imread/imread.h"
void tidylayer (layer* l,const Compute_float timemillis,const Compute_float* const gE,const Compute_float* const gI)
{
    if (Features.Recovery==OFF)
    {
        CalcVoltages(l->voltages,gE ,gI,l->P->potential,l->voltages_out);
        ResetVoltages(l->voltages_out,l->P->couple,l->firinglags,l->P->potential);
    }
    else
    {
        CalcRecoverys(l->voltages,l->recoverys,gE,gI,l->P->potential,l->P->recovery,l->voltages_out,l->recoverys_out);
    }
    StoreFiring(l);
    if (Features.Theta==ON)
    {
        dotheta(l->voltages_out,l->P->theta,timemillis);
    }
    if (Features.ImageStim==ON)
    {
        ApplyStim(l->voltages_out,timemillis,l->P->Stim);
    }
}
///Steps a model through 1 timestep - quite high-level function
///This is the only function in the file that needs model.h
void step1(model* m,const unsigned int time)
{
    const Compute_float timemillis = ((Compute_float)time) * Features.Timestep ;
    memset(m->gE,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size); //zero the gE/gI matrices so they can be reused for this timestep
    memset(m->gI,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size);
    if (Features.LocalStim==ON)
    {
        if (time %1000 < 250) {ApplyLocalBoost(m->gE,20,20);}
        else if (time % 1000 < 500) {ApplyLocalBoost(m->gE,20,60);}
        else if (time % 1000 < 750) {ApplyLocalBoost(m->gE,60,20);}
        else  {ApplyLocalBoost(m->gE,60,60);}
    }
    if(Features.UseAnimal==ON)
    {
        MoveAnimal(m->animal,timemillis);
        AnimalEffects(*m->animal,m->gE,time);
    }
    // Add spiking input to the conductances
    AddSpikes(m->layer1,m->gE,m->gI,time);
    if (m->NoLayers==DUALLAYER) {AddSpikes(m->layer2,m->gE,m->gI,time);}
    if (Features.Disablewrapping==OFF)
    {
        fixboundary(m->gE,m->gI);
    }
    // Add constant input to the conductances
    for (int i = 0;i < conductance_array_size*conductance_array_size;i++)
    {
        m->gE[i] += Extinput.gE0;
        m->gI[i] += Extinput.gI0;
    }
    //from this point the GE and GI are actually fixed - as a result there is no more layer interaction - so do things sequentially to each layer

    tidylayer(&m->layer1,timemillis,m->gE,m->gI);
    if (m->NoLayers==DUALLAYER){tidylayer(&m->layer2,timemillis,m->gE,m->gI);}
    if (Features.STDP==ON)
    {
        DoSTDP(m->layer1.connections,m->layer2.connections,m->layer1.STDP_data,m->layer1.P->STDP, m->layer2.STDP_data,m->layer2.P->STDP,m->layer1.rcinfo);
        DoSTDP(m->layer2.connections,m->layer1.connections,m->layer2.STDP_data,m->layer2.P->STDP, m->layer1.STDP_data,m->layer1.P->STDP,m->layer2.rcinfo);
    }
}
