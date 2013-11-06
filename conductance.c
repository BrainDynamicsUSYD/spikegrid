#include "matlab_includes.h"
#include "ringbuffer.h"
#include "parameters.h"
#include "helpertypes.h"
#include "coupling.h"
#include "STDP.h"
#include "STD.h"
#include "movie.h"
#include "output.h"
#include "assert.h"
#include <tgmath.h> //exp
#include <stdio.h> //printf
#include <stdlib.h> //malloc/calloc etc  random/srandom
#include <time.h>   //time - for seeding RNG
#include <string.h> //memcpy
#include <unistd.h> //gethostname
typedef struct layer
{
    Compute_float* connections;
    Compute_float* STDP_connections;
    Compute_float voltages[grid_size*grid_size]; //possibly these should be pointers so that things can be copied in/out a bit faster
    Compute_float voltages_out[grid_size*grid_size];
    Compute_float gE[conductance_array_size*conductance_array_size];
    Compute_float gI[conductance_array_size*conductance_array_size];
    coords_ringbuffer spikes;
} layer_t;

//creates a random initial condition - small fluctuations away from Vrt
void randinit(Compute_float* input)
{
    srandom((unsigned)(time(0)));
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = ((Compute_float)random())/((Compute_float)RAND_MAX)/20.0 + Param.potential.Vrt;
        }
    }
}


void setcaptests()
{   
    //todo:Get Adam to check these values
    assert (setcap(1.5,0.5,1E-6)==209);
    assert (setcap(2.0,0.5,1E-6)==270);
}

//add gE/gI when using STDP - untested
//when STDP is turned off, gcc will warn about this function needing const.  It is wrong
void evolvept_STDP  (const int x,const  int y,const Compute_float* const __restrict connections_STDP,const Compute_float Estrmod,const Compute_float Istrmod,Compute_float* __restrict gE,Compute_float* __restrict gI)
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
//rhs_func used when integrating the neurons forward through time
Compute_float __attribute__((const)) rhs_func  (const Compute_float V,const Compute_float gE,const Compute_float gI) {return -(Param.misc.glk*(V-Param.potential.Vlk) + gE*(V-Param.potential.Vex) + gI*(V-Param.potential.Vin));}
//step the model through time
void step1 (layer_t* layer,const int time)
{
    coords* current_firestore = layer->spikes.data[layer->spikes.curidx];//get the thing for currently firing neurons
    memset(layer->gE,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size); //zero the gE/gI matrices so they can be reused
    memset(layer->gI,0,sizeof(Compute_float)*conductance_array_size*conductance_array_size);
    for (int i=1;i<layer->spikes.count;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {
        coords* fire_with_this_lag;//this is a bit of a funny definition due to macros.
        RINGBUFFER_GETOFFSET(layer->spikes,i,fire_with_this_lag)
        const Compute_float delta = ((Compute_float)i)*Param.time.dt;//small helper constant
        const Compute_float Estr = (One/(Param.synapse.taudE-Param.synapse.taurE))*(exp(-delta/Param.synapse.taudE)-exp(-delta/Param.synapse.taurE));
        const Compute_float Istr = (One/(Param.synapse.taudI-Param.synapse.taurI))*(exp(-delta/Param.synapse.taudI)-exp(-delta/Param.synapse.taurI));
        int idx=0; //iterate through all neurons firing with this lag
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; //add conductances
            Compute_float strmod=One;
            if (Param.features.STD == ON)
            {
                const int stdidx=c.x*grid_size+c.y;
                if (i==1)
                {
                    const Compute_float dt = ((Compute_float)(time-STD.ftimes[stdidx]))/1000.0/Param.time.dt;//calculate inter spike interval in seconds
                    STD.ftimes[stdidx]=time; //update the time
                    const Compute_float prevu=STD.U[stdidx]; //need the previous U value
                    STD.U[stdidx] = Param.STD.U + STD.U[stdidx]*(One-Param.STD.U)*exp(-dt/Param.STD.F);
                    STD.R[stdidx] = One + (STD.R[stdidx] - prevu*STD.R[stdidx] - One)*exp(-dt/Param.STD.D);
                }
                strmod = STD.U[stdidx] * STD.R[stdidx] * 2.0; //multiplication by 2 is not in the cited papers, but you could eliminate it by multiplying some other parameters by 2, but multiplying by 2 here enables easier comparison with the non-STD model.  Max has an improvement that calculates a first-order approxiamation that should be included
            }
            evolvept(c.x,c.y,layer->connections,Estr*strmod,Istr*strmod,layer->gE,layer->gI,layer->STDP_connections);
            idx++;
        }
    }
    fixboundary(layer->gE,layer->gI);
    for (int x=0;x<grid_size;x++) 
    {
        for (int y=0;y<grid_size;y++)
        { //step all neurons through time - use midpoint method
            const int idx = (x+couplerange)*conductance_array_size + y + couplerange; //index for gE/gI
            const int idx2=  x*grid_size+y;//index for voltages
            const Compute_float rhs1=rhs_func(layer->voltages[idx2],layer->gE[idx],layer->gI[idx]);
            const Compute_float Vtemp = layer->voltages[idx2] + 0.5*Param.time.dt*rhs1;
            const Compute_float rhs2=rhs_func(Vtemp,layer->gE[idx],layer->gI[idx]);
            layer->voltages_out[idx2]=layer->voltages[idx2]+0.5*Param.time.dt*(rhs1 + rhs2);
        }
    }
    int this_fcount=0;//now find which neurons fired at the current time step
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            if (layer->voltages[x*grid_size + y]  > Param.potential.Vth)
            {
                const coords c= {x,y}; //required to keep the compiler happy - can't do an inline constructor
                current_firestore[this_fcount] =c;
                layer->voltages_out[x*grid_size+y]=Param.potential.Vrt;
                this_fcount++;
                if (Param.features.Output==ON)
                {
                    printf("%i,%i;",x,y);
                }
            }
            else if (((Compute_float)random())/((Compute_float)RAND_MAX) < (Param.misc.rate*0.001*Param.time.dt))
            {
                layer->voltages_out[x*grid_size+y]=Param.potential.Vth+0.1;//make sure it fires
            }
        }
    }
    //todo: fix for time scale properly
    for (int i=1;i<=51;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {   //put refractory neurons at reset potential
        coords* fire_with_this_lag;//this is a bit of a funny definition due to macros.
        RINGBUFFER_GETOFFSET(layer->spikes,i,fire_with_this_lag)
        int idx=0;
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; 
            layer->voltages_out[c.x*grid_size+c.y]=Param.potential.Vrt;
            idx++;
        }
    }
    current_firestore[this_fcount].x=-1;
    if (Param.features.Output==ON &&time % 10 ==0 ) {printf("\n");}

}
layer_t glayer;
//allocate memory - that sort of thing
void setup()
{
    couple_array_size=2*couplerange+1;
    //compute some constants
    int cap=max(setcap(Param.synapse.taudE,Param.synapse.taurE,1E-6),setcap(Param.synapse.taudI,Param.synapse.taurI,1E-6));
    //set up our data structure to store spikes
    glayer.spikes.count=cap;
    glayer.spikes.data=calloc(sizeof(coords*), cap);
    glayer.connections        = CreateCouplingMatrix();
    glayer.STDP_connections   = calloc(sizeof(Compute_float),grid_size*grid_size*couple_array_size*couple_array_size);
    memset(glayer.voltages,0,grid_size*grid_size);
    memset(glayer.voltages_out,0,grid_size*grid_size);
    for (int i=0;i<cap;i++)
    {
        glayer.spikes.data[i]=calloc(sizeof(coords),(grid_size*grid_size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
        glayer.spikes.data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
    }
    //for storing voltages
    if (Param.features.STD == ON) {STD_init();}

}
int mytime=0;
void matlab_step(const Compute_float* const inp)
{
    mytime++;
    memcpy(glayer.voltages,inp,sizeof(float)*grid_size*grid_size);
    glayer.spikes.curidx=mytime%(glayer.spikes.count);
    step1(&glayer,mytime);
    if (Param.features.STDP==ON)
    {
        doSTDP(glayer.STDP_connections,glayer.spikes,glayer.connections);
    }
    if (Param.features.Movie==ON &&  mytime % Param.Movie.Delay == 0) {printVoltage(glayer.voltages_out);}
   
}
int setup_done=0;
#ifdef MATLAB
//some classes for returning the data to matlab

output_s Outputabble[]={ //note - neat feature - missing elements initailized to 0
    {"gE",{glayer.gE,conductance_array_size,couplerange}}, //gE is a 'large' matrix - as it wraps around the edges
    {"gI",{glayer.gI,conductance_array_size,couplerange}}, //gE is a 'large' matrix - as it wraps around the edges
    {"R",{STD.R,grid_size}},
    {"U",{STD.R,grid_size}},
    {NULL}};         //a marker that we are at the end of the outputabbles list
//function called by matlab
//currently does no checking on input / output, so if you screw up your matlab expect segfaults
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
    if (setup_done==0) 
    {
        char* buffer = malloc(1024);
        gethostname(buffer,1023);
        if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
        printf("setup started\n");setup();setup_done=1;printf("done setup\n");
    }
    const Compute_float* inputdata = mxGetData(prhs[0]);
    matlab_step(inputdata);
    plhs[0]=mxCreateNumericMatrix(grid_size,grid_size,mxSINGLE_CLASS,mxREAL);
    Compute_float* pointer=(Compute_float*)mxGetPr(plhs[0]);
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            pointer[i*grid_size+j]=glayer.voltages_out[i*grid_size + j];
        }
        printf("%f\n",glayer.voltages_out[i*grid_size]);
    }
    if (nrhs>1)
    {
        int rhsidx = 1;
        while (rhsidx<nrhs)
        {
            char* data=malloc(sizeof(char)*1024);
            mxGetString(prhs[rhsidx],data,1023);
            int outidx = 0;
            while (Outputabble[outidx].name != NULL)
            {
                if (!strcmp(Outputabble[outidx].name,data))
                {
                    plhs[rhsidx]=outputToMxArray(Outputabble[outidx].data);
                    outidx=-1;
                    break;
                }
                outidx++;
            }
            if (outidx != -1) {printf("UNKNOWN THING TO OUTPUT\n");}
            free(data);
            rhsidx++;
        }
    }
}
#endif
int main()
{
    char* buffer = malloc(1024);
    gethostname(buffer,1023);
    if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
    setup();
    Compute_float* input=calloc(sizeof(Compute_float),grid_size*grid_size);
    randinit(input);
    while (mytime<100)
    {
        matlab_step(input);
        printf("%i\n",mytime);
        for (int i=0;i<grid_size;i++)
        {
            for (int j=0;j<grid_size;j++)
            {
                input[i*grid_size+j]=glayer.voltages_out[i*grid_size + j];
            }
        }
    }
   
    return EXIT_SUCCESS;
}
