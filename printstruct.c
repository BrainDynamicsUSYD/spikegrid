/// \file
/// Output a struct to a file and then print the value with gdb - massive hack.
/// based on http://stackoverflow.com/a/3312105/124259
#define _GNU_SOURCE //to get access to program_invocation_short_name
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
void printout_struct(const void* const invar, const char* const structname)
{
    char dbx[160];
    sprintf(dbx, "echo 'p (%s)*%p\n' > gdbcmds", structname, invar );
    system(dbx);

    sprintf(dbx, /*"echo 'where\ndetach' | */"gdb -batch --command=gdbcmds ./%s %d > struct.dump", program_invocation_short_name, getpid() );
	printf("%s",dbx);
    system(dbx);

    sprintf(dbx, "cat struct.dump");
    system(dbx);

    return;
}
