/// \file
#ifndef LAYER
#define LAYER
#ifdef __cplusplus
extern "C"
{
    #endif
#include "../enums.h" //needed at least for the is-inhibitory flag
#include "../simplestorage.h"
    //#include "../STDP.h"
    #ifdef __cplusplus
}
#endif
#include "../paramheader.h"
typedef struct STD_data STD_data; //forward declare STD_data to make things cleaner - makes this file a little messier, but it makes it more obvious where things come from
typedef struct STDP_data STDP_data;
    //   typedef struct STDP_data STDP_data; //forward declare STD_data to make things cleaner - makes this file a little messier, but it makes it more obvious where things come from
///hold the requisite data for a layer that enables it to be evolved through time.
typedef struct parameters parameters;
typedef struct randconns_info randconns_info;
typedef struct RD_data {
#ifdef __cplusplus
    RD_data(const decay_parameters in);
#endif

    Compute_float Rmat [conductance_array_size*conductance_array_size];
    Compute_float Dmat [conductance_array_size*conductance_array_size];
    const Compute_float R; 
    const Compute_float D;
} RD_data;

typedef struct in_out
{
    Compute_float In [grid_size*grid_size];
    Compute_float Out [grid_size*grid_size];
} in_out;

//the conmat and phimat here are structs because of trickery:
//we want essentially to create a const float[x]
//however, that can't be done.
//Instead, a 1 member struct is created.  We then have 2 steps
//1) in the 1 member struct constructor, it is OK to assign to the arrays
//2) the big struct then creates the small struct in the constructor and is allowed to assign to the const struct in the constructor
struct Conmat
{
    #ifdef __cplusplus
    Conmat(const couple_parameters c);
    #endif
    Compute_float connections[couple_array_size*couple_array_size];
};
struct Phimat
{
#ifdef __cplusplus
    Phimat(const parameters p, const model_features f);
    #endif
    Compute_float phimat[grid_size*grid_size];
};
typedef struct Phimat Phimat;
typedef struct Conmat Conmat;
struct layer
{
#ifdef __cplusplus
    layer(const parameters p,const model_features f,const int trefrac_in_ts);
    #endif
    const Phimat phimat;
    const Conmat connections;
    in_out voltages;
    in_out recoverys;
    randconns_info* rcinfo;
    simplestorage lags;
    STDP_data*      STDP_Data;
    parameters P;  /*TODO - would be nice for this parameters object to be const - should be doable just messes with the paramters object having every single field be const*/
    STD_data*       std;
    const on_off Layer_is_inhibitory;
    RD_data RD;
   } ;
typedef struct layer layer;
#endif
