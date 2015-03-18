//this file is compiled as C so that designated initializers work
#include "../cppparamheader.h"
#include "../enums.h"
const couple_parameters couple_example = {.Layertype = DUALLAYER,.Layer_parameters.dual={.connectivity=EXPONENTIAL,.W=1,.sigma=1}};
