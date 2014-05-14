#include <string.h> //memset
#include <stdlib.h> //random
#include "theta.h"
#include "evolve.h"
#include "STD.h"
//add gE/gI when using STDP - untested
//when STDP is turned off, gcc will warn about this function needing const.  It is wrong
void evolvept_STDP  (const int x,const  int y,const Compute_float* const __restrict connections_STDP,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI)
{
    if (Features.STDP == OFF) {return;}
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
void evolvept (const int x,const  int y,const Compute_float* const __restrict connections,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI,const Compute_float* STDP_CONNS)
{
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

void evolvept_singlelayer (const int x,const  int y,const Compute_float* const __restrict connections,const Compute_float strmod, Compute_float* __restrict condmat,const Compute_float* STDP_CONNS)
{
    for (int i = 0; i < couple_array_size;i++)
    {
        const int outoff = (x +i )*conductance_array_size +y;//as gE and gI are larger than the neuron grid size, don't have to worry about wrapping
        for (int j = 0 ; j<couple_array_size;j++) 
        {
            const int coupleidx = i*couple_array_size + j;
            condmat[outoff+j] += connections[coupleidx]*strmod;
        }
    } 
//    Need to find a way to do STDP here
}
void AddSpikes(layer L, Compute_float* __restrict__ gE, Compute_float* __restrict__ gI,const unsigned int time)
{
    for (unsigned int i=1;i<L.spikes.count;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {
        const coords* const fire_with_this_lag = ringbuffer_getoffset(&L.spikes,(int)i);
        const int delta =(int)(((Compute_float)i)*Features.Timestep);//small helper constant.
        const Compute_float Estr =L.Extimecourse!=NULL? L.Extimecourse[delta]:Zero;
        const Compute_float Istr =L.Intimecourse!=NULL? L.Intimecourse[delta]:Zero; 
        int idx=0; //iterate through all neurons firing with this lag
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; //add conductances
            Compute_float strmod=One;
            if (Features.STD == ON)
            {
                strmod=STD_str(L.P->STD,c.x,c.y,time,i,&(L.std));
            }
            if (Estr!= Zero && Istr != Zero) {evolvept(c.x,c.y,L.connections,Estr*strmod,Istr*strmod,gE,gI,L.STDP_connections);}
            else                             {evolvept_singlelayer(c.x,c.y,L.connections,(Estr==Zero?Istr*-1:Estr)*strmod,(Estr==Zero?gI:gE),L.STDP_connections);}
            idx++;
        }
    }
}

//To keep the evolvept code simple we use an array like this:
//     +––––––––––––-––––+          
//     |Extra for overlap|          
//     |  +––––––––––+   |          
//     |  |Actual    |   |          
//     |  |matrix    |   |          
//     |  +––––––––––+   |          
//     +–––––––––––––––-–+ 
//This function adds in the overlapping bits back into the original matrix.  It is slightly opaque but using pictures you can convince yourself that it works
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
//rhs_func used when integrating the neurons forward through time.  The actual integration is done using the midpoint method
Compute_float __attribute__((const,pure)) rhs_func  (const Compute_float V,const Compute_float ge,const Compute_float gi,const conductance_parameters p) {return -(p.glk*(V-p.Vlk) + ge*(V-p.Vex) + gi*(V-p.Vin));}

void CalcVoltages(const Compute_float* const __restrict__ Vinput,const Compute_float* const __restrict__ gE, const Compute_float* const __restrict__ gI,const conductance_parameters C, Compute_float* const __restrict__ Vout)
{
    for (int x=0;x<grid_size;x++) 
    {
        for (int y=0;y<grid_size;y++)
        { //step all neurons through time - use midpoint method
            const int idx = (x+couplerange)*conductance_array_size + y + couplerange; //index for gE/gI
            const int idx2=  x*grid_size+y;//index for voltages
            const Compute_float rhs1=rhs_func(Vinput[idx2],gE[idx],gI[idx],C);
            const Compute_float Vtemp = Vinput[idx2] + Half*Features.Timestep*rhs1;
            const Compute_float rhs2=rhs_func(Vtemp,gE[idx],gI[idx],C);
            Vout[idx2]=Vinput[idx2]+Half*Features.Timestep*(rhs1 + rhs2);
        }
    }
}

void StoreFiring(layer* L)
{
    coords* current_firestore = L->spikes.data[L->spikes.curidx];//get the thing for currently firing neurons
    int this_fcount=0;
    int step = L->skip;
    for (int x=0;x<grid_size;x+= skip)
    {
        for (int y=0;y<grid_size;y+=skip)
        {
            if (L->voltages[x*grid_size + y]  > L->P->potential.Vth)
            {
                current_firestore[this_fcount] =(coords){.x=x,.y=y};
                L->voltages_out[x*grid_size+y]=L->P->potential.Vrt;
                this_fcount++;
                if (Features.Output==ON)
                {
                    printf("%i,%i;",x,y);
                }
            }
            else if (((Compute_float)random())/((Compute_float)RAND_MAX) < 
                    (L->P->potential.rate*((Compute_float)0.001)*Features.Timestep))
            {
                L->voltages_out[x*grid_size+y]=L->P->potential.Vth+(Compute_float)0.1;//make sure it fires
            }
        }
    }
    if (Features.Output==ON ) {printf("\n");} //occasionaly print newlines
    current_firestore[this_fcount].x=-1;
}
void ResetVoltages(Compute_float* const __restrict Vout,const couple_parameters C,const ringbuffer* spikes,const conductance_parameters CP)
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
void step1(model* m,const unsigned int time)
{
	const Compute_float timemillis = ((Compute_float)time) * Features.Timestep ;
    memset(m->gE,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size); //zero the gE/gI matrices so they can be reused for this timestep
    memset(m->gI,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size);
    AddSpikes(m->layer1,m->gE,m->gI,time);
    if (m->NoLayers==DUALLAYER) {AddSpikes(m->layer2,m->gE,m->gI,time);}
    fixboundary(m->gE,m->gI);
    CalcVoltages(m->layer1.voltages,m->gE,m->gI,m->layer1.P->potential,m->layer1.voltages_out);
    if (m->NoLayers==DUALLAYER) {CalcVoltages(m->layer1.voltages,m->gE,m->gI,m->layer1.P->potential,m->layer1.voltages_out);}
    if (Features.Theta==ON)
    {
        dotheta(m->layer1.voltages_out,m->layer1.P->theta,timemillis);
        if (m->NoLayers==DUALLAYER) {dotheta(m->layer1.voltages_out,m->layer1.P->theta,timemillis);}
    }
    StoreFiring(&(m->layer1));
    if(m->NoLayers==DUALLAYER){StoreFiring(&(m->layer2));}
    ResetVoltages(m->layer1.voltages_out,m->layer1.P->couple,&m->layer1.spikes,m->layer1.P->potential);
    if(m->NoLayers==DUALLAYER){ResetVoltages(m->layer2.voltages_out,m->layer2.P->couple,&m->layer2.spikes,m->layer2.P->potential);}
}
