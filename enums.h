/// \file
#ifndef ENUMS
#define ENUMS
///Simple enum for things that are on or off to make their state more obvious
typedef enum {OFF=0,ON=1} on_off;
///Normalization method to use when creating a coupling matrix
typedef enum {None=0,TotalArea=1,GlobalMultiplier=2,MultSep=3} Norm_type;
/// The normalization method used in the 2009 paper.  This method normalizes by the total area of Ex and In connections
///Enum to determine the type of connectivity
typedef enum ConnectType {HOMOGENEOUS=1,EXPONENTIAL=0} ConnectType;
///Enum to determine how many layers are in use
typedef enum LayerNumbers {SINGLELAYER=0,DUALLAYER=1} LayerNumbers;
///Enum to determine whether there is a recovery variable
typedef enum NEURON_TYPE {LIF=0,QIF=1,EIF=2} neuron_type;
///Specify the destination of the output
typedef enum {NO_OUTPUT = 0,PICTURE = 1,TEXT=2,CONSOLE=3} output_method;
///Types of initial conditions that we have
typedef enum {RAND_TIME=0,RAND_JOB=1,RAND_ZERO=2,SINGLE_SPIKE=3} InitConds;
#endif
