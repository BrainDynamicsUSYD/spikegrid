/* this file is intended to be included by some of the whio test/demo code */
#include <stddef.h> /* size_t (at least on Linux) */
#include <string.h> /* memset() */
struct AppCommand;
typedef int (*AppCommand_f)( struct AppCommand const * self, void const * runData, size_t argc , char const * const * );
typedef struct AppCommand
{
    char const * name;
    char const * desc;
    char const * usage;
    AppCommand_f run;
    void const * runData;
    size_t argc_min;
    struct AppCommand const * subcommands;
} AppCommand;
#define AppCommand_empty_m {\
        NULL/*name*/,\
            "[command description]",\
            "[usage text]",                                             \
            NULL/*run*/,\
            NULL/*runData*/,                    \
            0/*argc_min*/,                      \
            NULL/*subcommands*/                 \
            }
static const AppCommand AppCommand_empty = AppCommand_empty_m;

enum AppErrors
    {
    AppCommandOK = 0,
    AppErrNYI = 1,
    AppErrMissingArgs,
    AppErrUsage,
    AppErrUnexpectedNull
    };

void AppCommand_show_help( AppCommand const * self, size_t level )
{
    if( ! self ) return;
    char tabs[level+1];
    memset( tabs, '\t', level );
    tabs[level] = 0;
    if( ! level )
    {
	printf("%sHelp for command: %s\n", tabs, self->name );
    }
    else
    {
	printf("%s[%s]:\n", tabs, self->name );
    }
    printf("%sDescription: %s\n", tabs, self->desc );
    printf("%sUsage: %s %s\n", tabs, self->name, self->usage );
    AppCommand const * c = self->subcommands;
    if( c && c->name )
    {
	printf("%sSubcommands:\n", tabs);//, self->name );
    }
    for( ; c && c->name; ++c )
    {
	AppCommand_show_help( c, level + 1 );
    }
    if( level > 0 ) puts("");
}

int AppCommand_try_subcommands_f( AppCommand const * self, void const * ignored, size_t argc, char const * const * argv )
{
    /*size_t i; i = self->subcommands ? 0 : argc;*/
    if( argc > 0 )
    {
	size_t c = 0;
	char const * cmd = argv[0];
	--argc; ++argv;
	AppCommand const * sub = (cmd && self->subcommands) ? self->subcommands : 0;
	for( ; sub && sub->name; sub = (self->subcommands + (++c)) )
	{
	    //MARKER("[%s] checking if cmd [%s] is subcommand [%s]...\n", self->name, cmd, sub->name );
	    if( 0 == strcmp(cmd,sub->name) )
	    {
		if( argc < sub->argc_min )
		{
		    MARKER(("Not enough arguments for the command '%s'.", sub->name ));
		    AppCommand_show_help( sub, 0 );
		    return whio_rc.ArgError;//AppErrUsage;
		}
		return sub->run
		    ? sub->run( sub, sub->runData, argc, argv )
		    : whio_rc.InternalError; //AppErrUnexpectedNull;
	    }
	}
    }
    AppCommand_show_help( self, 0 );
    //MARKER("command usage: %s %s\n", self->name, self->usage );
    return whio_rc.ArgError;//AppErrUsage;
}

