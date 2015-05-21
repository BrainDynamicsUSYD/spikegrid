///\file
#include <string.h> //memset
#include <stdio.h>  //useful when we need to print things
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

void RandSpikes(const unsigned int x,const unsigned int y,const layer L,Compute_float* __restrict__ gE, Compute_float* __restrict__ gI,const Compute_float str)
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

void evolvept (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI)
{
    for (int i = 0; i < couple_array_size;i++)
    {
        const int outoff = (x + i)*conductance_array_size +y;//as gE and gI are larger than he neuron grid size, don't have to worry about wrapping
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

///Adds the effect of the spikes that have fired in the past to the gE and gI arrays as appropriate
/// currently, single layer doesn't work (correctly)
void AddSpikes(layer L, Compute_float* __restrict__ gE, Compute_float* __restrict__ gI,const unsigned int time)
{
    for (unsigned int y=0;y<grid_size;y++)
    {
        for (unsigned int x=0;x<grid_size;x++)
        {

            const unsigned int lagidx = LagIdx(x,y,L.firinglags);
            unsigned int newlagidx = lagidx;
            
            if (L.Mytimecourse==NULL) // Single layer
            {

                    Compute_float excstr = Zero;
                    Compute_float inhstr = Zero;

                    while (L.firinglags->lags[newlagidx] != -1)  //Note: I think perf might be overstating the amount of time on this line - although, if it isn't massive potential for perf improvement
                    {
                        Compute_float this_excstr = L.Extimecourse[L.firinglags->lags[newlagidx]];
                        Compute_float this_inhstr = L.Intimecourse[L.firinglags->lags[newlagidx]];
                        if (Features.STD == ON)
                        {
                            this_excstr = this_excstr * STD_str(L.P->STD,x,y,time,L.firinglags->lags[newlagidx],L.std);
                            this_inhstr = this_inhstr * STD_str(L.P->STD,x,y,time,L.firinglags->lags[newlagidx],L.std);
                        }
                        newlagidx++;
                        excstr += this_excstr;
                        inhstr += this_inhstr;
                    }
                    if (newlagidx != lagidx) //only fire if we had a spike.
                    {
                        evolvept((int)x,(int)y,L.connections,excstr,inhstr,gE,gI); // No support for STDP in single layer  
                    }
            }
            else // Dual layer
            {
                Compute_float str = Zero;

                while (L.firinglags->lags[newlagidx] != -1)  //Note: I think perf might be overstating the amount of time on this line - although, if it isn't massive potential for perf improvement
                {
                    Compute_float this_str =L.Mytimecourse[L.firinglags->lags[newlagidx]];
                    if (Features.STD == ON)
                    {
                        this_str = this_str * STD_str(L.P->STD,x,y,time,L.firinglags->lags[newlagidx],L.std);
                    }
                    newlagidx++;
                    str += this_str;
                }
                if (L.Layer_is_inhibitory) {str = (-str);} //invert strength for inhib conns.
                if (newlagidx != lagidx) //only fire if we had a spike.
                {

                }
                if (Features.Random_connections == ON ) // No support for this in single layer
                {
                    RandSpikes(x,y,L,gE,gI,str);
                }
            } 

        }
    }
}
///This function adds in the overlapping bits back into the original matrix.  It is slightly opaque but using pictures you can convince yourself that it works
///To keep the evolvept code simple we use an array like this:
///~~~~
///     +-––––––––––––––––+
///     |Extra for Overlap|
///     |  +––––––––––+   |
///     |  |Actual    |   |
///     |  |matrix    |   |
///     |  +––––––––––+   |
///     +–––––––––––––––––+
///~~~~
void fixboundary(Compute_float* __restrict input)
{   //theoretically the two sets of loops could be combined but that would be incredibly confusing
    //in particular, the left and right sides can get a     little confused
    //The zeroing is required for the D/R step
    if (Features.Disablewrapping==ON) {return;} //as this function does wrapping, bail if it isn't on.
    //top + bottom
    for (int i=0;i<couplerange;i++)
    {
        for (int j=0;j<conductance_array_size;j++)
		{
            input[(grid_size+i)*conductance_array_size + j] += input[i*conductance_array_size+j]; //add to bottom
            input[i*conductance_array_size+j] = 0;
            input[(i+couplerange)*conductance_array_size+j] += input[(grid_size+couplerange+i)*conductance_array_size+j];//add to top
            input[(grid_size+couplerange+i)*conductance_array_size+j] = 0;
		}
	}
    //left + right boundary condition fix
    for (int i=couplerange;i<couplerange+grid_size;i++)
    {
        for (int j=0;j<couplerange;j++)

		{
             input[i*conductance_array_size    +grid_size+j ]  += input[i*conductance_array_size+j];//left
             input[i*conductance_array_size+j]=0;//left
             input[i*conductance_array_size +couplerange+j] += input [i*conductance_array_size + grid_size+couplerange+j];//right
             input [i*conductance_array_size + grid_size+couplerange+j]=0;//right
		}
	}
}
//Ideally we change things so that this isn't required - maybe an inline function to get gE/gI using Rvalues
//note - we need to use the normalised D/R values when we add the initial numbner
void FixRD(Compute_float* __restrict R,const Compute_float Rm,Compute_float* __restrict D, const Compute_float Dm,Compute_float* __restrict gE, Compute_float* __restrict gI,const on_off inhib)
{
    fixboundary(R);
    fixboundary(D);
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            const int idx = Conductance_index(i,j);
            R[idx] *= exp(-Features.Timestep/Rm);
            D[idx] *= exp(-Features.Timestep/Dm);
            if (inhib==ON)
            {
                gI[idx] += - ( D[idx]-R[idx]);
            }
            else
            {
                gE[idx] += D[idx]-R[idx]; //question - calculate this first or second?
            }
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
//detect if a neuron is active - may be useful elsewhere - used to maintain an appropriate ratio of ex/in neurons
//note when we have STDP, if you have one layer with skip +x and another with -x the code is massively simpler.
//+ve skip is obvious.  -ve skip does the "inverse" of +ve skip
int __attribute__((pure,const)) IsActiveNeuron (const int x, const int y,const signed char step)
{
    const char test = (x % step) == 0 && (y % step) ==0;
    return (test && step > 0) || (!test && step < 0);
}

///Store current firing spikes also apply random spikes
///TODO: make faster - definitely room for improvement here
void StoreFiring(layer* L)
{
    const signed char skip = (signed char) (L->P->skip);
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            if (IsActiveNeuron(x,y,skip))
            {
                const unsigned int baseidx = LagIdx((unsigned int)x,(unsigned int)y,L->firinglags);
                modifyLags(L->firinglags,baseidx);
                //now - add in new spikes
                //TODO: restore STDP spike storage
                if (L->voltages_out[x*grid_size + y]  >= L->P->potential.Vpk)
                {
                    AddnewSpike(L->firinglags,baseidx);
                    if (Features.Recovery==ON) //reset recovery if needed.  Note recovery has no refractory period so a reset is required
                    {
                        L->voltages_out[x*grid_size+y]=L->P->potential.Vrt;
                        L->recoverys_out[x*grid_size+y]+=L->P->recovery.Wrt;
                    }
                    if (L->Layer_is_inhibitory == ON)
                    {
                        AddRD(x,y,L->connections, L->Rmat,L->Dmat,L->R,L->D);
                    }
                    else
                    {

                        AddRD(x,y,L->connections, L->Rmat,L->Dmat,L->R,L->D);
                    }
                }//add random spikes
                else if (L->P->potential.rate > 0 && //this check is because the compiler doesn't optimize the call to random() otherwise
                            (RandFloat() < (L->P->potential.rate*((Compute_float)0.001)*Features.Timestep)))
                {
                    L->voltages_out[x*grid_size+y]=L->P->potential.Vpk+(Compute_float)0.1;//make sure it fires - the neuron will actually fire next timestep
                    AddRD(x,y,L->connections,L-> Rmat,L->Dmat,L->R,L->D);
                }
            }
            else //non-active neurons never get to fire
            {
                L->voltages_out[x*grid_size+y]=-1000; //skipped neurons set to -1000 - probably not required but perf impact should be minimal - also ensures they will never be >Vpk
            }
        }
    }
}
///Cleans up voltages for neurons that are in the refractory state
void RefractoryVoltages(Compute_float* const __restrict Vout,const couple_parameters C,const lagstorage* const  l,const conductance_parameters CP)
{
    const int trefrac_in_ts =(int) ((Compute_float)C.tref / Features.Timestep);
    for (unsigned int i=0;i<grid_size*grid_size;i++)
    {
        unsigned int baseidx = i*l->lagsperpoint;
        if (CurrentShortestLag(l,baseidx) <= trefrac_in_ts)
        {
            Vout[i] = CP.Vrt;
        }
    }
}
#include "imread/imread.h"
///This function takes up way too much time in the code - mostly in storefiring - slightly annoying as this is essentially all pure overhead.  It would be really nice to significantly reduce the amount of time this function takes.
void tidylayer (layer* l,const Compute_float timemillis,const Compute_float* const gE,const Compute_float* const gI)
{
    if (Features.Recovery==OFF)
    {
        CalcVoltages(l->voltages,gE ,gI,l->P->potential,l->voltages_out);
        if (Features.ImageStim==ON) //Dodgy fudge
        {
            if (!(l->P->Stim.Periodic))
            {
            ApplyContinuousStim(l->voltages_out,timemillis,l->P->Stim,Features.Timestep,l->Phimat);
            }
        }
        RefractoryVoltages(l->voltages_out,l->P->couple,l->firinglags,l->P->potential);
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
        if (l->P->Stim.Periodic)
        {
            ApplyStim(l->voltages_out,timemillis,l->P->Stim,l->P->potential.Vpk,l->STDP_data);
        }
    }
}
///Steps a model through 1 timestep - quite high-level function
///This is the only function in the file that needs model.h
void step1(model* m)
{
    const Compute_float timemillis = ((Compute_float)m->timesteps) * Features.Timestep ;
    //this memcpy based version for initializing gE/gI is marginally slower (probably cache issues) - 
    memcpy(m->gE,m->gEinit,sizeof(m->gE));
    memcpy(m->gI,m->gIinit,sizeof(m->gI));
    if (Features.LocalStim==ON)
    {
        if (m->timesteps %1000 < 250) {ApplyLocalBoost(m->gE,20,20);}
        else if (m->timesteps % 1000 < 500) {ApplyLocalBoost(m->gE,20,60);}
        else if (m->timesteps % 1000 < 750) {ApplyLocalBoost(m->gE,60,20);}
        else  {ApplyLocalBoost(m->gE,60,60);}
    }
    if(Features.UseAnimal==ON)
    {
        MoveAnimal(m->animal,timemillis);
        AnimalEffects(*m->animal,m->gE,timemillis);
    }
    // Add spiking input to the conductances
    AddSpikes(m->layer1,m->gE,m->gI,m->timesteps);
    if (m->NoLayers==DUALLAYER) {AddSpikes(m->layer2,m->gE,m->gI,m->timesteps);}
    //from this point the GE and GI are actually fixed - as a result there is no more layer interaction - so do things sequentially to each layer
    if (m->NoLayers==DUALLAYER)
    {
        FixRD(m->layer1.Rmat,m->layer1.R,m->layer1.Dmat,m->layer1.D,m->gE,m->gI,m->layer1.Layer_is_inhibitory);
        FixRD(m->layer2.Rmat,m->layer2.R,m->layer2.Dmat,m->layer2.D,m->gE,m->gI,m->layer2.Layer_is_inhibitory);
    }
    else
    {
        fixboundary(m->gE);
        fixboundary(m->gI);
    }
    tidylayer(&m->layer1,timemillis,m->gE,m->gI);
    if (m->NoLayers==DUALLAYER){tidylayer(&m->layer2,timemillis,m->gE,m->gI);}
    if (Features.STDP==ON)
    {
        DoSTDP(m->layer1.connections,m->layer2.connections,m->layer1.STDP_data,m->layer2.STDP_data,m->layer1.rcinfo);
        DoSTDP(m->layer2.connections,m->layer1.connections,m->layer2.STDP_data,m->layer1.STDP_data,m->layer2.rcinfo);
    }
}
