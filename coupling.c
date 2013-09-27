#include "parameters.h"
#include "coupling.h" //not actually required at the moment but should ensure that function types match
#include <math.h> //logf / exp
#include <stdlib.h> //calloc
/*typedef struct coupling
{
    float* excouple;
    float* incouple
};*/
float erange;
float exrange()
{
   return -(Param.couple.sigE*Param.couple.sigI*logf(Param.couple.WE/Param.couple.WI))/(Param.couple.sigE-Param.couple.sigI); //from mathematica
}

//check how far back we need to keep track of histories
int setcap(float D,float R,float minval)
{
    float prev = -1000;//initial values
    float alpha = 0;
    float time=0;
    float norm=1.0/(D-R);
    while(1)
    {
        time+=Param.time.dt;
        alpha=(exp(-time/D) - exp(-time/R))*norm;
        // check that the spike is in the decreasing phase and that it has magnitude less than the critical value
        if (alpha<minval && alpha<prev) {break;}
        prev=alpha;
    }
    return (int)(time/Param.time.dt) +1; //this keeps compatibility with the matlab - seems slightly inelegent - maybe remove
}

//compute the mexican hat function used for coupling
float mexhat(const float rsq){return Param.couple.WE*exp(-rsq/Param.couple.sigE)-Param.couple.WI*exp(-rsq/Param.couple.sigI);}

//does what it says on the tin
float* CreateCouplingMatrix()
{
    float* matrix = calloc(sizeof(float),couple_array_size*couple_array_size); //matrix of coupling values
    erange=exrange();
    for(int x=-couplerange;x<=couplerange;x++)
    {
        for(int y=-couplerange;y<=couplerange;y++)
        {
            if (x*x+y*y<=couplerange*couplerange)//if we are within coupling range
            {
                float val = mexhat(x*x+y*y);//compute the mexican hat function
                if (val>0) {val=val*Param.couple.SE;} else {val=val*Param.couple.SI;}//and multiply by some constants
                matrix[(x+couplerange)*couple_array_size + y + couplerange] = val;//and set the array
            }
        }
    }
    return matrix;
}

