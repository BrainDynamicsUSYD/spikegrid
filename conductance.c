#include "ringbuffer.h"
#include "parameters.h"
#include "helpertypes.h"
#include "coupling.h"
#include "STDP.h"
#include "STD.h"
#include "assert.h"
#include <math.h> //exp
#include <stdio.h> //printf
#include <stdlib.h> //malloc/calloc etc  random/srandom
#include <time.h>   //time - for seeding RNG
#include <string.h> //memcpy
#include <unistd.h> //gethostname
//calculate how far back histories need to be kept for the tauR/tauD values we have picked.  As this number is typically ~ 200-300, finding it exactly will provide a nice speed boot
#ifdef MATLAB
    #include "mex.h" //matlab
    #include "matrix.h"  //matlab
#endif
float* STDP_connections;
//creates a random initial condition - small fluctuations away from Vrt
void randinit(float* input)
{
    srandom((unsigned)(time(0)));
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            input[x*grid_size + y ] = ((float)random())/((float)RAND_MAX)/20.0 + Param.potential.Vrt;
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
void evolvept_STDP (const int x,const  int y,const float* const __restrict connections_STDP,const float Estrmod,const float Istrmod,float* __restrict gE,float* __restrict gI)
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
void evolvept (const int x,const  int y,const float* const __restrict connections,const float Estrmod,const float Istrmod,float* __restrict gE,float* __restrict gI,const float* STDP_CONNS)
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
float rhs_func (const float V,const float gE,const float gI) {return -(Param.misc.glk*(V-Param.potential.Vlk) + gE*(V-Param.potential.Vex) + gI*(V-Param.potential.Vin));}
float gE[conductance_array_size*conductance_array_size]; //gE/gI matrices are reused in each call to minimise allocations
float gI[conductance_array_size*conductance_array_size];
//step the model through time
void step1 ( const float* const __restrict connections,coords_ringbuffer* fdata,const float* const __restrict input,float* output,const int time)
{
    coords* current_firestore = fdata->data[fdata->curidx];//get the thing for currently firing neurons
    memset(gE,0,sizeof(float)*conductance_array_size*conductance_array_size); //zero the gE/gI matrices so they can be reused
    memset(gI,0,sizeof(float)*conductance_array_size*conductance_array_size);
    for (int i=1;i<fdata->count;i++) //start at 1 so we don't get currently firing (which should be empty anyway)
    {
        coords* fire_with_this_lag;//this is a bit of a funny definition due to macros.
        RINGBUFFER_GETOFFSET(*fdata,i,fire_with_this_lag)
        const float delta = (float)(i*Param.time.dt);//small helper constant
        const float Estr = (1.0/(Param.synapse.taudE-Param.synapse.taurE))*(exp(-delta/Param.synapse.taudE)-exp(-delta/Param.synapse.taurE));
        const float Istr = (1.0/(Param.synapse.taudI-Param.synapse.taurI))*(exp(-delta/Param.synapse.taudI)-exp(-delta/Param.synapse.taurI));
        int idx=0; //iterate through all neurons firing with this lag
        while (fire_with_this_lag[idx].x != -1)
        {
            coords c = fire_with_this_lag[idx]; //add conductances
            if (Param.features.STD == ON)
            {
                const int stdidx=c.x*grid_size+c.y;
                const float dt = ((float)(time-STD.ftimes[stdidx]))/1000.0/Param.time.dt;//calculate inter spike interval in seconds
                STD.ftimes[stdidx]=time; //update the time
                const float prevu=STD.U[stdidx]; //need the previous U value
                STD.U[stdidx] = Param.STD.U + STD.U[stdidx]*(1.0-Param.STD.U)*exp(-dt/Param.STD.F);
                STD.R[stdidx] = 1.0 + (STD.R[stdidx] - prevu*STD.R[stdidx] - 1.0)*exp(-dt/Param.STD.D);
                const float strmod = STD.U[stdidx] * STD.R[stdidx] * 2.0; //multiplication by 2 is not in the cited papers, but you could eliminate it by multiplying some other parameters by 2, but multiplying by 2 here enables easier comparison with the non-STD model
                //TODO: I don't like how I have 2 different calls to evolvept - need better solution.
                evolvept(c.x,c.y,connections,Estr*strmod,Istr*strmod,gE,gI,STDP_connections);
            }
            else
            {
                evolvept(c.x,c.y,connections,Estr,Istr,gE,gI,STDP_connections);
            }
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
            const float Vtemp = input[idx2] + 0.5*Param.time.dt*rhs1;
            const float rhs2=rhs_func(Vtemp,      gE[idx],gI[idx]);
            output[idx2]=input[idx2]+0.5*Param.time.dt*(rhs1 + rhs2);
        }
    }
    int this_fcount=0;//now find which neurons fired at the current time step
    for (int x=0;x<grid_size;x++)
    {
        for (int y=0;y<grid_size;y++)
        {
            if (input[x*grid_size + y]  > Param.potential.Vth)
            {
                const coords c= {x,y}; //required to keep the compiler happy - can't do an inline constructor
                current_firestore[this_fcount] =c;
                output[x*grid_size+y]=Param.potential.Vrt;
                this_fcount++;
                if (Output==ON)
                {
                    printf("%i,%i;",x,y);
                }
            }
            else if (((float)random())/((float)RAND_MAX) < (Param.misc.rate*0.001*Param.time.dt))
            {
                output[x*grid_size+y]=Param.potential.Vth+0.1;//make sure it fires
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
            output[c.x*grid_size+c.y]=Param.potential.Vrt;
            idx++;
        }
    }
    current_firestore[this_fcount].x=-1;
    if (Output==ON) {printf("\n");}

}

coords_ringbuffer* spikes;
float* connections;
//allocate memory - that sort of thing
void setup()
{
    couple_array_size=2*couplerange+1;
    //compute some constants
    int cap=max(setcap(Param.synapse.taudE,Param.synapse.taurE,1E-6),setcap(Param.synapse.taudI,Param.synapse.taurI,1E-6));
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
    STDP_connections = calloc(sizeof(float),grid_size*grid_size*couple_array_size*couple_array_size);
    if (Param.features.STD == ON) {STD_init();}

}
int mytime=0;
void matlab_step(const float* inp)
{
    mytime++;
    spikes->curidx=mytime%(spikes->count);
    step1(connections,spikes,inp,potentials2,mytime);
    if (Param.features.STDP==ON)
    {
        doSTDP(STDP_connections,spikes,connections);
    }
   
}
int setup_done=0;
#ifdef MATLAB
//some classes for returning the data to matlab
typedef enum print_type_e {FLOAT=1} print_type;
typedef enum array_size_e {LARGE=2} array_size;
typedef struct output_s 
{
    char* name;
    void* data;
    print_type type;
    array_size size;
} output;
output Outputabble[]={
    {"gE",gE,FLOAT,LARGE}, //gE is a 'large' matrix - as it wraps around the edges
    {"gI",gI,FLOAT,LARGE}, //same for gI
    {NULL,0,0,0}};         //a marker that we are at the end of the outputabbles list
//this function is incredibly messy.  
mxArray* print(output out)
{
    char* data = out.data; //use char as sizeof(char)==1 always
    int vsize;
    mxArray* ret;
    switch (out.type)
    {
        case FLOAT: //need to know how much data to copy and what sort of matlab array to create
            ret = mxCreateNumericMatrix(grid_size,grid_size,mxSINGLE_CLASS,mxREAL);
            vsize=sizeof(float);
            break;
    }
    char* datapointer=(char*)mxGetPr(ret);//use char for same reasons as above
    switch (out.size)
    {   //TODO: add normal array size
        case LARGE:
            for (int i=0;i<grid_size;i++)
            {
                for (int j=0;j<grid_size;j++)
                {   //I think this line is correct - it is very messy
                    memcpy(&datapointer[vsize*(i*grid_size+j)],&data[((i+couplerange)*conductance_array_size + j + couplerange)*vsize],vsize);
                }
            }
            break;

    }
    return ret;
}
//function called by matlab
//currently does no checking on input / output, so if you screw up your matlab expect segfaults
void mexFunction(int nlhs,mxArray *plhs[],int nrhs, const mxArray *prhs[])
{
    if (setup_done==0) 
    {
        char* buffer = malloc(1024);
        gethostname(buffer,1023);
        if (!strcmp(buffer,"headnode.physics.usyd.edu.au")) {printf("DON'T RUN THIS CODE ON HEADNODE\n");exit(EXIT_FAILURE);}
        setup();setup_done=1;printf("done setup\n");
    }
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
                    plhs[rhsidx]=print(Outputabble[outidx]);
                    outidx=-1;
                    break;
                }
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
    float* input=calloc(sizeof(float),grid_size*grid_size);
    randinit(input);
    while (mytime<100)
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
