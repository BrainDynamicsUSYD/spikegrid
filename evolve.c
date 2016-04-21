///\file
#include <string.h> //memset
#include <stdio.h>  //useful when we need to print things
#include "paramheader.h" //in VS this needs to be early to compile - don't know why
#include "enums.h"
#include "STDP.h"
#include "model.h"
#include "theta.h"
#include "evolvegen.h"
#include "STD.h"
#include "mymath.h"
#include "localstim.h"
#include "animal.h"
#include "randconns.h"
#include "lagstorage.h"
#ifdef ANDROID
    #define APPNAME "myapp"
    #include <android/log.h>
#endif

void evolvept (const coords c ,const Compute_float* const __restrict connections,const Compute_float Estrmod,const Compute_float Istrmod,condmat* __restrict cond_mat)
{
    for (int i = 0; i < couple_array_size;i++)
    {
        const int outoff = (c.x + i)*conductance_array_size +c.y;//as gE and gI are larger than he neuron grid size, don't have to worry about wrapping
        for (int j = 0 ; j<couple_array_size;j++)
        {
            const int coupleidx = i*couple_array_size + j;
            if (connections[coupleidx] > 0)
            {
                cond_mat->gE[outoff+j] += connections[coupleidx]*Estrmod;
            }
            else
            {
                cond_mat->gI[outoff+j] += -(connections[coupleidx]*Istrmod);
            }
        }
    }
}

///Adds the effect of the spikes that have fired in the past to the gE and gI arrays as appropriate
/// currently, single layer doesn't work (correctly)
void AddSpikes_single_layer(layer L, condmat* __restrict__ cond_mat,const unsigned int time)
{
    for (Neuron_coord y=0;y<grid_size;y++)
    {
        for (Neuron_coord x=0;x<grid_size;x++)
        {
            const coords c = {.x=x,.y=y};
            const size_t lagidx = LagIdx(c,L.firinglags);
            size_t newlagidx = lagidx;
            if (L.Mytimecourse==NULL) // Single layer TODO: change this if to use something more appropriate
            {
                Compute_float excstr = Zero;
                Compute_float inhstr = Zero;

                while (L.firinglags->lags[newlagidx] != -1)  //Note: I think perf might be overstating the amount of time on this line - although, if it isn't massive potential for perf improvement
                {
                    Compute_float this_excstr = L.Extimecourse[L.firinglags->lags[newlagidx]];
                    Compute_float this_inhstr = L.Intimecourse[L.firinglags->lags[newlagidx]];
                    if (Features.STD == ON)
                    {
                        this_excstr = this_excstr * STD_str(L.P->STD,c,time,L.firinglags->lags[newlagidx],L.std);
                        this_inhstr = this_inhstr * STD_str(L.P->STD,c,time,L.firinglags->lags[newlagidx],L.std);
                    }
                    newlagidx++;
                    excstr += this_excstr;
                    inhstr += this_inhstr;
                }
                if (newlagidx != lagidx) //only fire if we had a spike.
                {
                    evolvept(c,L.connections,excstr,inhstr,cond_mat); // No support for STDP in single layer
                }
            }
            else // Dual layer
            {
                printf("Addspikes_single called for a dual layer model - quitting\n");
                exit(EXIT_FAILURE);
            }
        }
    }
}
///This function adds in the overlapping bits back into the original matrix.  It is slightly opaque but using pictures you can convince yourself that it works
///To keep the evolvept code simple we use an array like this:
///since the change to the D and R method this trick is less useful, for perf but makes the code much simpler and still does give some perf.
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
void FixRD(RD_data* __restrict RD,condmat* __restrict cond_mat,const on_off inhib)
{
    fixboundary(RD->Rmat);
    fixboundary(RD->Dmat);
    for (Neuron_coord i=0;i<grid_size;i++)
    {
        for (Neuron_coord j=0;j<grid_size;j++)
        {
            const size_t idx = Conductance_index((coords){.x=i,.y=j});
            RD->Rmat[idx] *= exp(-Features.Timestep/RD->R);
            RD->Dmat[idx] *= exp(-Features.Timestep/RD->D);
            if (inhib==ON)
            {
                cond_mat->gI[idx] += -(RD->Dmat[idx]-RD->Rmat[idx]);
            }
            else
            {
                cond_mat->gE[idx] += RD->Dmat[idx]-RD->Rmat[idx]; //question - calculate this first or second?
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
        const condmat * const __restrict__ cond_mat,
        const conductance_parameters C,
        Compute_float* const __restrict__ Vout)
{
    for (Neuron_coord x=0;x<grid_size;x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {
            const coords c = {.x=x,.y=y};
            const size_t idx =Conductance_index(c);
            const size_t idx2=  grid_index(c);
            const Compute_float rhs = rhs_func(Vinput[idx2],cond_mat->gE[idx],cond_mat->gI[idx],C);
            Vout[idx2]=Vinput[idx2]+Features.Timestep*rhs;
        }
    }
}
///Uses precalculated gE and gI to integrate the voltages and recoverys forward through time. This uses the Euler method
//this should probably take a struct as input - way too many arguments
void CalcRecoverys(const Compute_float* const __restrict__ Vinput,
        const Compute_float* const __restrict__ Winput,
        const condmat* const __restrict__ cond_mat,
        const conductance_parameters C,
        const recovery_parameters R,
        Compute_float* const __restrict__ Vout,
        Compute_float* const __restrict__ Wout)
{    // Adaptive quadratic integrate-and-fire
    for (Neuron_coord x=0;x<grid_size;x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {   //step all neurons through time - use Euler method
            const coords c = {.x=x,.y=y};
            const size_t idx =Conductance_index(c);
            const size_t idx2=  grid_index(c);
            const Compute_float rhsV=rhs_func(Vinput[idx2],cond_mat->gE[idx],cond_mat->gI[idx],C)-Winput[idx2];
            const Compute_float rhsW=R.Wcv*(R.Wir*(Vinput[idx2]-C.Vlk) - Winput[idx2]);
            Vout[idx2] = Vinput[idx2] + Features.Timestep*rhsV;
            Wout[idx2] = Winput[idx2] + Features.Timestep*rhsW;
        }
    }
}
//detect if a neuron is active - may be useful elsewhere - used to maintain an appropriate ratio of ex/in neurons
//note when we have STDP, if you have one layer with skip +x and another with -x the code is massively simpler.
//+ve skip is obvious.  -ve skip does the "inverse" of +ve skip
//this function here could maybe use the coords - but it is very time sensitive, so leave as is
int __attribute__((pure,const)) IsActiveNeuron (const int x, const int y,const signed char step)
{
    const char test = (x % step) == 0 && (y % step) ==0;
    return (test && step > 0) || (!test && step < 0);
}
void AddRandomRD(const coords c ,const randconns_info* const rcinfo, RD_data* __restrict RD,const Compute_float InitStr)
{
    unsigned int count;
    const randomconnection* const rcsleaving = GetRandomConnsLeaving(c,*rcinfo,&count);
    for (unsigned int i=0;i<count;i++)
    {
        const randomconnection rc = rcsleaving[i];
        const size_t destidx = Conductance_index(rc.destination);
        RD->Rmat[destidx] += InitStr*(rc.strength+rc.stdp_strength);
        RD->Dmat[destidx] += InitStr*(rc.strength+rc.stdp_strength);
    }
}
///Store current firing spikes also apply random spikes
///TODO: make faster - definitely room for improvement here
void StoreFiring(layer* L,const unsigned int timestep)
{
    const signed char skip = (signed char) (L->P->skip);
    for (Neuron_coord x=0;x<grid_size;x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {
            if (IsActiveNeuron(x,y,skip))
            {
                const coords coord = {.x=x,.y=y};
                const size_t baseidx = LagIdx(coord,L->firinglags);
                modifyLags(L->firinglags,baseidx,grid_index(coord));
                if (Features.STDP==ON) {modifyLags(L->STDP_data->lags,LagIdx(coord,L->STDP_data->lags),grid_index(coord));}
                //now - add in new spikes
                if (L->voltages_out[grid_index(coord)]  >= L->P->potential.Vpk
                        ||
                        //next part - add random spikes
                        (L->P->potential.rate > 0 && //this check is because the compiler doesn't optimize the call to random() otherwise
                         (RandFloat() < (L->P->potential.rate*((Compute_float)0.001)*Features.Timestep)))

                   )
                {
                    AddnewSpike(L->firinglags,baseidx);
                    if (Features.STDP==ON && L->STDP_data->RecordSpikes==ON) {AddnewSpike(L->STDP_data->lags,LagIdx(coord,L->STDP_data->lags));}
                    if (Features.Recovery==ON) //reset recovery if needed.  Note recovery has no refractory period so a reset is required
                    {
                        L->voltages_out[grid_index(coord)]=L->P->potential.Vrt;
                        L->recoverys_out[grid_index(coord)]+=L->P->recovery.Wrt;
                    }
                    Compute_float spikestr = 1.0/(L->RD->D - L->RD->R);
                    if (Features.STD==ON)
                    {
                        spikestr = spikestr * STD_str(L->P->STD,coord,timestep,1,L->std); //since we only emit a spike once, use 1 to force an update
                    }
                    //all the different ways to add a spike
                    if (Features.STDP==OFF)
                    {
                        //TODO: maybe the Addrd functions should take coords
                        AddRD(coord,L->connections, L->RD,spikestr);
                    }
                    else
                    {
                        AddRD_STDP(coord,L->connections,L->STDP_data->connections,L->RD,spikestr);
                    }
                    if (Features.Random_connections==ON)
                    {
                        AddRandomRD(coord,L->rcinfo,L->RD,spikestr);
                    }
                }
            }
            else //non-active neurons never get to fire
            {
                L->voltages_out[x*grid_size+y]=-1000; //skipped neurons set to -1000 - probably not required but perf impact should be minimal - also ensures they will never be >Vpk  - removing this would make the pictures smoother
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
        if (CurrentShortestLag(l,baseidx,i) <= trefrac_in_ts)
        {
            Vout[i] = CP.Vrt;
        }
    }
}
#include "imread/imread.h"
///This function takes up way too much time in the code - mostly in storefiring - slightly annoying as this is essentially all pure overhead.  It would be really nice to significantly reduce the amount of time this function takes.
void tidylayer (layer* l,const Compute_float timemillis,const condmat* const __restrict cond_mat,const unsigned int timestep)
{
    if (Features.Recovery==OFF)
    {
        CalcVoltages(l->voltages,cond_mat,l->P->potential,l->voltages_out);
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
        CalcRecoverys(l->voltages,l->recoverys,cond_mat,l->P->potential,l->P->recovery,l->voltages_out,l->recoverys_out);
    }
    StoreFiring(l,timestep);
    if (Features.Theta==ON)
    {
        dotheta(l->voltages_out,l->P->theta,timemillis);
    }
    if (Features.ImageStim==ON)
    {
        if (l->P->Stim.Periodic==ON)
        {
            ApplyStim(l->voltages_out,timemillis,l->P->Stim,l->P->potential.Vpk,l->STDP_data,l->rcinfo);
        }
    }
    if (l->P->STDP.STDP_decay_frequency>0 && (int)(timemillis/Features.Timestep) % l->P->STDP.STDP_decay_frequency == 0)
    {
        printf("decaying STDP\n");
        STDP_decay(l->STDP_data,l->rcinfo);
    }

}
///Steps a model through 1 timestep - quite high-level function
///This is the only function in the file that needs model.h
void step1(model* m)
{
    const Compute_float timemillis = ((Compute_float)m->timesteps) * Features.Timestep ;
    //this memcpy based version for initializing gE/gI is marginally slower (probably cache issues) -
    memcpy(m->cond_matrices,m->cond_matrices_init,sizeof(*m->cond_matrices));
    if (Features.LocalStim==ON)
    {
        if (m->timesteps %1000 < 250) {ApplyLocalBoost(m->cond_matrices->gE,20,20);}
        else if (m->timesteps % 1000 < 500) {ApplyLocalBoost(m->cond_matrices->gE,20,60);}
        else if (m->timesteps % 1000 < 750) {ApplyLocalBoost(m->cond_matrices->gE,60,20);}
        else  {ApplyLocalBoost(m->cond_matrices->gE,60,60);}
    }
    if(Features.UseAnimal==ON)
    {
        MoveAnimal(m->animal,timemillis);
        AnimalEffects(*m->animal,m->cond_matrices->gE,timemillis);
    }
    // Add spiking input to the conductances
    if (m->NoLayers==SINGLELAYER) {AddSpikes_single_layer(m->layer2,m->cond_matrices,m->timesteps);}
    //from this point the GE and GI are actually fixed - as a result there is no more layer interaction - so do things sequentially to each layer
    if (m->NoLayers==DUALLAYER)
    {
        FixRD(m->layer1.RD,m->cond_matrices,m->layer1.Layer_is_inhibitory);
        FixRD(m->layer2.RD,m->cond_matrices,m->layer2.Layer_is_inhibitory);
    }
    else
    {
        fixboundary(m->cond_matrices->gE);
        fixboundary(m->cond_matrices->gI);
    }
    tidylayer(&m->layer1,timemillis,m->cond_matrices,m->timesteps);
    if (m->NoLayers==DUALLAYER){tidylayer(&m->layer2,timemillis,m->cond_matrices,m->timesteps);}
    if (Features.STDP==ON)
    {
        DoSTDP(m->layer1.connections,m->layer2.connections,m->layer1.STDP_data,m->layer2.STDP_data,m->layer1.rcinfo);
        DoSTDP(m->layer2.connections,m->layer1.connections,m->layer2.STDP_data,m->layer1.STDP_data,m->layer2.rcinfo);
    }
}
