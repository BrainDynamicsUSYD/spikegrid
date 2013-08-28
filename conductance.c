#include "ringbuffer.h"
#include "parameters.h"
#include "helpertypes.h"
#include "coupling.h"
#include "assert.h"
#include <math.h> //exp
#include <stdio.h> //printf
#include <stdlib.h> //malloc/calloc etc  random/srandom
#include <time.h>   //time - for seeding RNG
#include <string.h>
//calculate how far back histories need to be kept for the tauR/tauD values we have picked.  As this number is typically ~ 200-300, finding it exactly will provide a nice speed boot
#ifdef MATLAB
    #include "mex.h" //matlab
    #include "matrix.h"  //matlab
#endif
RINGBUFFER_DEF(coords)
//creates a random initial condition - small fluctuations away from Vrt
void randinit(float* input)
{
    srandom((unsigned)(time(0)));
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = ((float)random())/((float)RAND_MAX)/20.0 + Vrt;
        }
    }
}


void setcaptests()
{   
    //todo:Get Adam to check these values
    assert (setcap(1.5,0.5,1E-6)==209);
    assert (setcap(2.0,0.5,1E-6)==270);
}

//add conductance from a firing neuron the gE and gI arrays
////TODO: add the skip part to skip zero entries as in the threestate code
void evolvept (const int x,const  int y,const float* const __restrict connections,const float Estrmod,const float Istrmod,float* __restrict gE,float* __restrict gI)
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
}
void fixboundary(float* __restrict gE, float* __restrict gI)
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
float rhs_func (const float V,const float gE,const float gI) {return -(glk*(V-Vlk) + gE*(V-Vex) +gI*(V-Vin));}
float gE[conductance_array_size*conductance_array_size]; //gE/gI matrices are reused in each call to minimise allocations
float gI[conductance_array_size*conductance_array_size];
void step1 ( const float* const __restrict connections,coords_ringbuffer* fdata,const float* const __restrict input,float* output,const int time)
{
    coords* current_firestore = fdata->data[fdata->curidx];//get the thing for currently firing neurons
    memset(gE,0,sizeof(float)*conductance_array_size*conductance_array_size);
    memset(gI,0,sizeof(float)*conductance_array_size*conductance_array_size);
    for (int i=1;i<fdata->count;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {
        coords* fire_with_this_lag;//this is a bit of a funny definition due to macros.
        RINGBUFFER_GETOFFSET(*fdata,i,fire_with_this_lag)
        const float delta = (float)(i*dt);//small helper constant
        const float Estr = (1.0/(taudE-taurE))*(exp(-delta/taudE)-exp(-delta/taurE));
        const float Istr = (1.0/(taudI-taurI))*(exp(-delta/taudI)-exp(-delta/taurI));
        int idx=0; //iterate through all neurons firing with this lag
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; //add conductances
            evolvept(c.x,c.y,connections,Estr,Istr,gE,gI);
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
            const float rhs1=rhs_func(input[idx2],gE[idx],gI[idx]);
            const float Vtemp = input[idx2] + 0.5*dt*rhs1;
            const float rhs2=rhs_func(Vtemp,      gE[idx],gI[idx]);
            output[idx2]=input[idx2]+0.5*dt*(rhs1 + rhs2);
        }
    }
    int this_fcount=0;//now find which neurons fired at the current time step
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            if (input[x*grid_size + y]  > Vth)
            {
                const coords c= {x,y}; //required to keep the compiler happy - can't do an inline constructor
                current_firestore[this_fcount] =c;
                output[x*grid_size+y]=Vrt;
                this_fcount++;
            }
            else if (((float)random())/((float)RAND_MAX) < (rate*0.001*dt))
            {
                output[x*grid_size+y]=Vth+0.1;//make sure it fires
            }
        }
    }
    //todo: fix for time scale properly
    for (int i=1;i<=51;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {   //put refractory neurons at reset potential
        coords* fire_with_this_lag;//this is a bit of a funny definition due to macros.
        RINGBUFFER_GETOFFSET(*fdata,i,fire_with_this_lag)
        int idx=0;
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; 
            output[c.x*grid_size+c.y]=Vrt;
            idx++;
        }
    }
    current_firestore[this_fcount].x=-1;
    
}

coords_ringbuffer* spikes;
float* connections;
void setup()
{
    couple_array_size=2*couplerange+1;
    //compute some constants
    steps = duration/dt;
    int cap=max(setcap(taudE,taurE,1E-6),setcap(taudI,taurI,1E-6));
    //set up our data structure to store spikes
    spikes = malloc(sizeof(coords_ringbuffer));
    spikes->count = cap;
    spikes->data=calloc(sizeof(coords*),cap);
    for (int i=0;i<cap;i++)
    {
        spikes->data[i]=calloc(sizeof(coords),(grid_size*grid_size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
        spikes->data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
    }
    //for storing voltages
    potentials=calloc(sizeof(float),grid_size*grid_size);
    potentials2=calloc(sizeof(float),grid_size*grid_size);
    connections=CreateCouplingMatrix();

}
int mytime=0;
void matlab_step(const float* inp)
{
    mytime++;
    spikes->curidx=mytime%(spikes->count);
    step1(connections,spikes,inp,potentials2,mytime);
   
}
int setup_done=0;
#ifdef MATLAB
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
    if (setup_done==0) {setup();setup_done=1;printf("done setup\n");}
    const float* inputdata = mxGetData(prhs[0]);
    matlab_step(inputdata);
    plhs[0]=mxCreateNumericMatrix(grid_size,grid_size,mxSINGLE_CLASS,mxREAL);
    float* pointer=(float*)mxGetPr(plhs[0]);
    for (int i=0;i<grid_size;i++)
    {
        for (int j=0;j<grid_size;j++)
        {
            pointer[i*grid_size+j]=potentials2[i*grid_size + j];
        }
    }
}
#endif
int main()
{
    setup();
    float* input=calloc(sizeof(float),grid_size*grid_size);
    randinit(input);
    while (mytime<1000)
    {
        matlab_step(input);
        for (int i=0;i<grid_size;i++)
        {
            for (int j=0;j<grid_size;j++)
            {
                input[i*grid_size+j]=potentials2[i*grid_size + j];
            }
        }
    }
   
    return EXIT_SUCCESS;
}
