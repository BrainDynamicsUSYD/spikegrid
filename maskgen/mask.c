/// \file
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "../sizes.h" //so we get the correct parameter values
int __attribute__((const)) min(const int a,const int b) {if (a<b) {return a;} else {return b;}}
//calculate the circle
int __attribute__((const)) getoffset(const int range, const int i)
{
    const double drange = (double)range;
    const double di     = (double)(i-range);
    if (drange < di) {return (int)drange;}
    return min(range,(int) ((sqrt(drange*drange-di*di)) ));
}
//virtually identical to below - please keep both methods in sync - need to improve
void withSTDP()
{
    //some initial setup lines
    printf("void evolvept_duallayer_STDP (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float* const __restrict STDP_connections,const Compute_float strmod, Compute_float* __restrict condmat)\n");
    printf("{\n");
    //start the loop
    for (int i = 0; i < couple_array_size;i++)
    {
        printf("    const int outoff%i = ((x+%i)*%i + y);\n",i,i,conductance_array_size);
        printf("    const int conoff%i = %i*couple_array_size;\n",i,i);
        printf("    const int STDPoff%i = (x*grid_size+y)*couple_array_size*couple_array_size + conoff%i;\n" ,i,i);
        char buf [1000];
        sprintf(buf,"((x+%i)*%i + y)",i,conductance_array_size);
        const int off = getoffset(couplerange,i);
        printf("    for (int kk=(couplerange-%i);kk<=(couplerange+%i);kk++)\n",off,off); //this is the key part - offsets are now known at compile time
        printf("    {\n");
        printf("        condmat[outoff%i + kk] += (connections[conoff%i+kk] + STDP_connections[STDPoff%i+kk])*strmod;\n",i,i,i); //include base connection and STDP-modified connection
        printf("    }\n");
    }
    printf("}\n");

}
void checkfn()
{
    printf("void check()\n");
    printf("{\n");
    printf("    if (conductance_array_size != %i || couplerange != %i || couple_array_size != %i)\n",conductance_array_size,couplerange,couple_array_size);
    printf("    {\n");
    printf("        printf(\"You need to regenerate maskgen.c\");\n");
    printf("        exit(EXIT_FAILURE);\n");
    printf("    }\n");
    printf("}");

}
int main()
{
    //some initial setup lines
    printf("//autogenerated code - do not modify - see maskgen\n");
    printf("//parameters: conductance_array_size %i, couplerange %i, couple_array_size %i\n",conductance_array_size,couplerange,couple_array_size);
    printf("#include \"sizes.h\"\n");
    printf("#include \"typedefs.h\"\n");
    printf("#include <stdio.h>\n");
    printf("#include <stdlib.h>\n");
    printf("long long int c  = 0;\n");
    printf("void evolvept_duallayer (const int x,const int y,const Compute_float* const __restrict connections,const Compute_float strmod, Compute_float* __restrict condmat)\n");
    printf("{\n");
    printf("c++;");
    //start the loop
    for (int i = 0; i < couple_array_size;i++)
    {
        printf("    const int outoff%i = ((x+%i)*%i + y);\n",i,i,conductance_array_size);
        printf("    const int conoff%i = %i*couple_array_size;\n",i,i);
        char buf [1000];
        sprintf(buf,"((x+%i)*%i + y)",i,conductance_array_size);
        const int off = getoffset(couplerange,i);
        printf("    for (int kk=(couplerange-%i);kk<=(couplerange+%i);kk++)\n",off,off); //this is the key part - offsets are now known at compile time
        printf("    {\n");
        printf("        condmat[outoff%i + kk] += connections[conoff%i+kk]*strmod;\n",i,i);
        printf("    }\n");
    }
    printf("}\n");
    withSTDP();
    checkfn();
}
