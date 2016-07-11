/// \file
//make a directory and all the parts
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <malloc.h>
#include "VS/VS/Winheader.h"
//massive ugly hack.  However, this does appear to be the simplest way to recursively
//create a directory
void recursive_mkdir(const char* const dirname)
{
    char mkcmd[500];
    sprintf(mkcmd, "mkdir -p %s", dirname);
    system(mkcmd);
}
///Copies a struct using malloc - occasionally required
/// @param input  the initial input
/// @param size   the amount of data to copy
void* newdata(const void* const input,const unsigned int size)
{
    void* ret = malloc(size);
    memcpy(ret,input,size);
    return ret;
}
#if defined(ANDROID)|| defined(_WIN32)
void Hook_malloc() {} //do nothing
#else
long long int total_malloced;
//need to disable a warning for the rest of the file.  TODO: find a better solution than __malloc_hook
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
static void *(*old_malloc_hook)(size_t,const void *);
//this function is based on something in the GCC docs

static void * my_malloc_hook (size_t size, __attribute__((unused)) const void * caller)
{
  void *result;
  // Restore all old hooks
  __malloc_hook = old_malloc_hook;
  // Call recursively - have to have the hook disabled
  result = malloc (size);
  total_malloced += (long long int)size;
  if (result==NULL)
  {
      printf("Malloc failed - returned NULL - quitting early\n");
      printf("This is probably because you have insufficient RAM.  Try shrinking the grid size or disabling STDP/random connections\n");
      exit(EXIT_FAILURE);
  }
  // Restore our own hooks */
  __malloc_hook = my_malloc_hook;

  return result;
}
//set up malloc hook.  With multiple jobs, this can be called multiple times, so only change the hook once.
void Hook_malloc()
{
    if (__malloc_hook != my_malloc_hook)
    {
        old_malloc_hook = __malloc_hook;
        __malloc_hook = my_malloc_hook;
    }
}
#endif
