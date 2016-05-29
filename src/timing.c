#include <time.h>
#include <stdio.h>
#include "typedefs.h"
struct timespec curtime;
Compute_float starttimeNS;
void inittimer()
{
    clock_gettime(CLOCK_MONOTONIC,&curtime); //we want a monotonic clock here to avoid problems
    starttimeNS=(Compute_float)curtime.tv_sec*(Compute_float)1e9+(Compute_float)curtime.tv_nsec;
}
void timertick(const size_t timesteps,const size_t totaltimesteps)
{
    clock_gettime(CLOCK_MONOTONIC,&curtime);
    Compute_float nanosecondsNOW = (Compute_float)curtime.tv_nsec + (Compute_float)(curtime.tv_sec)*(Compute_float)1e9; //This is the most ridiculous stupid api ever.  Just use a bigger data type people
    Compute_float timeperstep = 1.0/((nanosecondsNOW-starttimeNS) /(Compute_float)timesteps/(Compute_float)1e9);
    Compute_float secondstofinish = ((Compute_float)(totaltimesteps-timesteps))/timeperstep;
    printf("%i timesteps / second.  Finishing in %i seconds, %i%% done\n",
            (int)timeperstep, //print everything as ints to make the display a little nicer - don't need perfect accuraccy
            (int)secondstofinish,
            (int)((Compute_float)timesteps/(Compute_float)totaltimesteps*100.0));

}
