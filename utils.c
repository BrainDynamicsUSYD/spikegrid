/// \file
//make a directory and all the parts
#include <stdio.h>
#include <stdlib.h>
//massive ugly hack.  However, this does appear to be the simplest way to recursively
//create a directory
void recursive_mkdir(const char* const dirname)
{
    char mkcmd[500];
    sprintf(mkcmd, "mkdir -p %s", dirname);
    system(mkcmd);
}
