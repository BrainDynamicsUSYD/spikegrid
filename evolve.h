/// \file
typedef struct model model;
void step1 (model* m);
#ifdef TESTING //for unit testing the code
int IsActiveNeuron (const int x, const int y,const int step);
#endif
