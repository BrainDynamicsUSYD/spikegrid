/// \fileman strtok
//make a directory and all the parts
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
void recursive_mkdir(char* dirname)
{
    char buf[100];
    
    const char delim[2] = "/";
    char* token = strtok(dirname,delim); //first call - set up tokenisation
    while (token!=NULL)
    {
        strcat(buf,"/");
        strcat(buf,token); //concat strings
        mkdir(buf,S_IRWXU); //make subdirectory
        token=strtok(NULL,delim); //get next directory
    }
}