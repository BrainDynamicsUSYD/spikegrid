#include "ringbuffer.h"
#include "parameters.h"
#include "helpertypes.h"
#include "assert.h"
#include <math.h> //exp
#include <stdio.h> //printf
#include <stdlib.h> //malloc/calloc etc  random/srandom
#include <time.h>   //time - for seeding RNG
//calculate how far back histories need to be kept for the tauR/tauD values we have picked.  As this number is typically ~ 200-300, finding it exactly will provide a nice speed boot
#ifdef MATLAB
    #include "mex.h" //matlab
    #include "matrix.h"  //matlab
#endif
RINGBUFFER_DEF(coords)

//check how far back we need to keep track of histories
int setcap(float D,float R,float minval)
{
    float prev = -1000;//initial values
    float alpha = 0;
    float time=0;
    float norm=1.0/(D-R);
    while(1)
    {
        time+=dt;
        alpha=(exp(-time/D) - exp(-time/R))*norm;
        // check that the spike is in the decreasing phase and that it has magnitude less than the critical value
        if (alpha<minval && alpha<prev) {break;}
        prev=alpha;
    }
    return (int)(time/dt) +1; //this keeps compatibility with the matlab - seems slightly inelegent - maybe remove
}

//compute the mexican hat function used for coupling
float mexhat(const float rsq){return WE*exp(-rsq/sigE)-WI*exp(-rsq/sigI);}

//does what it says on the tin
float* CreateCouplingMatrix()
{
    float* matrix = calloc(sizeof(float),couple_array_size*couple_array_size); //matrix of coupling values
    for(int x=-couplerange;x<=couplerange;x++)
    {
        for(int y=-couplerange;y<=couplerange;y++)
        {
            if (x*x+y*y<=couplerange*couplerange)//if we are within coupling range
            {
                float val = mexhat(x*x+y*y);//compute the mexican hat function
                if (val>0) {val=val*SE;} else {val=val*SI;}//and multiply by some constants
                matrix[(x+couplerange)*couple_array_size + y + couplerange] = val;//and set the array
                printf("%f,",val);
            }
            else {printf("0,");}
        }
        printf("\n");
    }
    return matrix;
}
//creates a random initial condition - small fluctuations away from Vrt
void randinit(float* input)
{
    srandom((unsigned)(time(0)));
    for (int x=0;x<size;x++)
    {
        for (int y=0;y<size;y++)
        {
            input[x*size + y ] = ((float)random())/((float)RAND_MAX)/20.0 + Vrt;
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
void evolvept (const int x,const  int y,const float* const __restrict connections,const float Estrmod,const float Istrmod,float* __restrict gE,float* __restrict gI)
{
    int i,j;
    for (i = 0; i < couple_array_size;i++)
    {
        const int outoff = (x +i)*conductance_array_size +y;//as gE and gI are larger than the neuron grid size, don't have to worry about wrapping
        for (j =0 ; j<couple_array_size;j++) 
        {
            const int coupleidx = i*couple_array_size + j;
            if((gE[outoff+j]<0.0) || (gI[outoff+j]<0.0)){printf("ERROR\n");}
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
            gE[(size+i)*conductance_array_size + j] +=gE[i*conductance_array_size+j]; //add to bottom
            gE[(i+couplerange)*conductance_array_size+j] += gE[(size+couplerange+i)*conductance_array_size+j];//add to top
            gI[(size+i)*conductance_array_size + j] +=gI[i*conductance_array_size+j]; //add to bottom
            gI[(i+couplerange)*conductance_array_size+j] += gI[(size+couplerange+i)*conductance_array_size+j];//add to top
		}
	}
    //left + right boundary condition fix
    for (int i=couplerange;i<couplerange+size;i++)
	{
        for (int j=0;j<couplerange;j++)
		{
             gE[i*conductance_array_size +size+j ] += gE[i*conductance_array_size+j];//left
             gE[i*conductance_array_size +couplerange+j] += gE [i*conductance_array_size + size+couplerange+j];//right
             gI[i*conductance_array_size +size+j ] += gI[i*conductance_array_size+j];//left
             gI[i*conductance_array_size +couplerange+j] += gI [i*conductance_array_size + size+couplerange+j];//right
		}
	}

}
//rhs_func used when integrating the neurons forward through time
float rhs_func (const float V,const float gE,const float gI) {return -(glk*(V-Vlk) + gE*(V-Vex) +gI*(V-Vin));}

void step1 ( const float* const __restrict connections,coords_ringbuffer* fdata,const float* const __restrict input,float* output,const int time)
{
    coords* current_firestore = fdata->data[fdata->curidx];//get the thing for currently firing neurons
    float* gE     = calloc(sizeof(float),conductance_array_size*conductance_array_size);//arrays to store gE and gI - larger than neurons grid size
    float* gI     = calloc(sizeof(float),conductance_array_size*conductance_array_size);
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
    for (int x=0;x<size;x++) 
    {
        for (int y=0;y<size;y++)
        { //step all neurons through time - use midpoint method
            const int idx = (x+couplerange)*conductance_array_size + y + couplerange; //index for gE/gI
            const int idx2=  x*size+y;//index for voltages
            const float rhs1=rhs_func(input[idx2],gE[idx],gI[idx]);
            const float Vtemp = input[idx2] + 0.5*dt*rhs1;
            const float rhs2=rhs_func(Vtemp,      gE[idx],gI[idx]);
            output[idx2]=input[idx2]+0.5*dt*(rhs1 + rhs2);
        }
    }
    free(gE);free(gI);//don't need these matrices any more so throw them away. TODO: Even better - reuse them
    int this_fcount=0;//now find which neurons fired at the current time step
    for (int x=0;x<size;x++)
    {
        for (int y=0;y<size;y++)
        {
            if (input[x*size + y]  > Vth)
            {
                const coords c= {x,y}; //required to keep the compiler happy - can't do an inline constructor
                current_firestore[this_fcount] =c;
                output[x*size+y]=Vrt;
                this_fcount++;
            }
            else if (((float)random())/((float)RAND_MAX) < (rate*0.001*dt))
            {
                output[x*size+y]=Vth+0.1;//make sure it fires
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
            output[c.x*size+c.y]=Vrt;
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
    conductance_array_size=size+2*couplerange;
    //compute some constants
    steps = duration/dt;
    int cap=max(setcap(taudE,taurE,1E-6),setcap(taudI,taurI,1E-6));
    //set up our data structure to store spikes
    spikes = malloc(sizeof(coords_ringbuffer));
    spikes->count = cap;
    spikes->data=calloc(sizeof(coords*),cap);
    for (int i=0;i<cap;i++)
    {
        spikes->data[i]=calloc(sizeof(coords),(size*size + 1));//assume worst case - all neurons firing.  Need to leave spae on the end for the -1 which marks the end.
        spikes->data[i][0].x=-1;//need to make sure that we don't start with spikes by ending at 0
    }
    //for storing voltages
    potentials=calloc(sizeof(float),size*size);
    potentials2=calloc(sizeof(float),size*size);
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
    plhs[0]=mxCreateNumericMatrix(size,size,mxSINGLE_CLASS,mxREAL);
    float* pointer=mxGetPr(plhs[0]);
    for (int i=0;i<size;i++)
    {
        for (int j=0;j<size;j++)
        {
            pointer[i*size+j]=potentials2[i*size + j];
        }
    }
}
#endif
int main()
{
    setup();
    float* input=calloc(sizeof(float),size*size);
    randinit(input);
    while (mytime<1000)
    {
        matlab_step(input);
        for (int i=0;i<size;i++)
        {
            for (int j=0;j<size;j++)
            {
                input[i*size+j]=potentials2[i*size + j];
            }
        }
    }
   
    return EXIT_SUCCESS;
}
