///\file
#include <string.h> //memset
#include <stdio.h>  //useful when we need to print things
#include "paramheader.h" //in VS this needs to be early to compile - don't know why
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


void AddSpikes_single_layer(__attribute__((unused))layer L,__attribute__((unused)) condmat* __restrict__ cond_mat,__attribute__((unused))const unsigned int time)
{
    printf("single layer is currently unsupported\n");
    //does nothing - if you want to see what used to be here use git blame - but it wasn't particularly good anyway.
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
        {   //the indices through this function are a little messy - but kind of stuck that way
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
void CalcVoltages(in_out* const __restrict__ Voltages,
        const condmat * __restrict__ cond_mat,
        const conductance_parameters C,
        const int skip)
{
    for (Neuron_coord x=0;x<grid_size;skip>0?x+=skip:x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {
            const coords c = {.x=x,.y=y};
            const size_t idx =Conductance_index(c);
            const size_t idx2=  grid_index(c);
            const Compute_float rhs = rhs_func(Voltages->In[idx2],cond_mat->gE[idx],cond_mat->gI[idx],C);
            Voltages->Out[idx2]=Voltages->In[idx2]+Features.Timestep*rhs;
        }
    }
}
///Uses precalculated gE and gI to integrate the voltages and recoverys forward through time. This uses the Euler method
//this should probably take a struct as input - way too many arguments
void CalcRecoverys(in_out* const __restrict__ Voltages,
        in_out* const __restrict__ Recoveries,
        const condmat* const __restrict__ cond_mat,
        const conductance_parameters C,
        const recovery_parameters R)
{    // Adaptive quadratic integrate-and-fire
    for (Neuron_coord x=0;x<grid_size;x++)
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {   //step all neurons through time - use Euler method
            const coords c = {.x=x,.y=y};
            const size_t idx =Conductance_index(c);
            const size_t idx2=  grid_index(c);
            const Compute_float rhsV=rhs_func(Voltages->In[idx2],cond_mat->gE[idx],cond_mat->gI[idx],C)-Recoveries->In[idx2];
            const Compute_float rhsW=R.Wcv*(R.Wir*(Voltages->In[idx2]-C.Vlk) - Recoveries->In[idx2]);
            Voltages->Out[idx2] = Voltages->In[idx2] + Features.Timestep*rhsV;
            Recoveries->Out[idx2] = Recoveries->In[idx2] + Features.Timestep*rhsW;
        }
    }
}
//detect if a neuron is active - may be useful elsewhere - used to maintain an appropriate ratio of ex/in neurons
//note when we have STDP, if you have one layer with skip +x and another with -x the code is massively simpler.
//+ve skip is obvious.  -ve skip does the "inverse" of +ve skip
//this function here could maybe use the coords - but it is very time sensitive, so leave as is
//seems to take about 20% of the time
signed char __attribute__((pure,const)) IsActiveNeuron (const Neuron_coord x, const Neuron_coord y,const signed char step)
{   //with a bit of paper you can convince yourself that this code does what it is supposed to
    const signed char test = (x % step) == 0 && (y % step) ==0;
    return (test && step > 0) || (!test && step < 0); //here is the nice symettry we get with negative steps
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
    for (Neuron_coord x=0;x<grid_size;skip>0?x+=skip :x++) //this fancy skipping can make a pretty massive speed difference
    {
        for (Neuron_coord y=0;y<grid_size;y++)
        {
            if ( IsActiveNeuron(x,y,skip))
            {
                const coords coord = {.x=x,.y=y};
                if (Features.STDP==ON) {modifyLags(L->STDP_data->lags,LagIdx(coord,L->STDP_data->lags),grid_index(coord));}
                //now - add in new spikes
                if (L->voltages.Out[grid_index(coord)]  >= L->P->potential.Vpk
                        ||
                        //next part - add random spikes
                        (L->P->potential.rate > 0 && //this check is because the compiler doesn't optimize the call to random() otherwise
                         (RandFloat() < (L->P->potential.rate*((Compute_float)0.001)*Features.Timestep)))

                   )
                {
                    AddnewSpike_simple(grid_index(coord),&L->lags);
                    if (Features.STDP==ON && L->STDP_data->RecordSpikes==ON) {AddnewSpike(L->STDP_data->lags,LagIdx(coord,L->STDP_data->lags));}
                    if (Features.Recovery==ON) //reset recovery if needed.  Note recovery has no refractory period so a reset is required
                    {
                        L->voltages.Out[grid_index(coord)]=L->P->potential.Vrt;
                        L->recoverys.Out[grid_index(coord)]+=L->P->recovery.Wrt;
                    }
                    Compute_float spikestr = 1.0/(L->RD.D - L->RD.R); //This normalizes the base size strength so that changing D/R only changes the timescale but leaves the integral constant
                    if (Features.STD==ON)
                    {
                        spikestr = spikestr * STD_str(L->P->STD,coord,timestep,1,L->std); //since we only emit a spike once, use 1 to force an update
                    }
                    //all the different ways to add a spike
                    if (Features.STDP==OFF)
                    {
                        AddRD(coord,L->connections, &L->RD,spikestr);
                    }
                    else
                    {
                        AddRD_STDP(coord,L->connections,L->STDP_data->connections,&L->RD,spikestr);
                    }
                    if (Features.Random_connections==ON)
                    {
                        AddRandomRD(coord,L->rcinfo,&L->RD,spikestr);
                    }
                }
            }
            else //non-active neurons never get to fire
            {
                L->voltages.Out[x*grid_size+y]=-1000; //skipped neurons set to -1000 - probably not required but perf impact should be minimal - also ensures they will never be >Vpk  - removing this would make the pictures smoother
            }
        }
    }
}
///Cleans up voltages for neurons that are in the refractory state
void RefractoryVoltages(Compute_float* const __restrict Vout,simplestorage* s,const conductance_parameters CP)
{
    for (unsigned int i=0;i<grid_size*grid_size;i++)
    {
        if (s->lags[i] > 0)
        {
            Vout[i] = CP.Vrt;
            s->lags[i]--;
        }
    }
}
#include "imread/imread.h"
///This function takes up way too much time in the code - mostly in storefiring - slightly annoying as this is essentially all pure overhead.  It would be really nice to significantly reduce the amount of time this function takes.
void tidylayer (layer* l,const Compute_float timemillis,const condmat* const __restrict cond_mat,const unsigned int timestep)
{
    if (Features.Recovery==OFF)
    {
        CalcVoltages(&l->voltages,cond_mat,l->P->potential,l->P->skip);
        if (Features.ImageStim==ON) //Dodgy fudge
        {
            if (!(l->P->Stim.Periodic))
            {
                ApplyContinuousStim(l->voltages.Out,timemillis,l->P->Stim,Features.Timestep,l->Phimat);
            }
        }
        RefractoryVoltages(l->voltages.Out,&l->lags,l->P->potential);
    }
    else
    {
        CalcRecoverys(&l->voltages,&l->recoverys,cond_mat,l->P->potential,l->P->recovery);
    }
    StoreFiring(l,timestep);
    if (Features.Theta==ON)
    {
        dotheta(l->voltages.Out,l->P->theta,timemillis);
    }
    if (Features.ImageStim==ON)
    {
        if (l->P->Stim.Periodic==ON) //only stim excit layer
        {
            ApplyStim(l->voltages.Out,timemillis,l->P->Stim,l->P->potential.Vpk,l->STDP_data,l->rcinfo,l->Layer_is_inhibitory);
        }
    }
    if (l->P->STDP.STDP_decay_frequency>0 && (int)(timemillis/Features.Timestep) % l->P->STDP.STDP_decay_frequency == 0 && l->STDP_data->RecordSpikes==ON)
    {
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
    if (Features.LocalStim==ON) //this isn't really used anymore
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
        FixRD(&m->layer1.RD,m->cond_matrices,m->layer1.Layer_is_inhibitory);
        FixRD(&m->layer2.RD,m->cond_matrices,m->layer2.Layer_is_inhibitory);
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
        DoSTDP(m->layer1.connections,m->layer2.connections,m->layer1.STDP_data,m->layer2.STDP_data,m->layer1.rcinfo,m->layer2.rcinfo);
        DoSTDP(m->layer2.connections,m->layer1.connections,m->layer2.STDP_data,m->layer1.STDP_data,m->layer2.rcinfo,m->layer1.rcinfo);
    }
}
