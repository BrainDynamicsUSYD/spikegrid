/// \fileman strtok
//make a directory and all the parts
#include <stdio.h>
#include <stdlib.h>
void recursive_mkdir(const char* const dirname)
{
    char mkcmd[500];
    sprintf(mkcmd, "mkdir -p %s", dirname);
    system(mkcmd);
}