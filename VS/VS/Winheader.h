#pragma once
//Windows doesn't support some attributes - so define them in their own file
#ifdef _WIN32
#define __attribute__(...)
#define inline
#define __restrict__
#define srandom(a) srand(a)
#define random() rand()
#endif
