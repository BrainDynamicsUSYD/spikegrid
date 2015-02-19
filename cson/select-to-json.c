/**
   Test/demo code for the cson library.
*/
#if defined(NDEBUG)
/* force assert() to always work */
#  undef NDEBUG
#endif

#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* getenv() */
#include <string.h> /*strlen() and friends. */
#include <inttypes.h> /* PRIu32 and friends. */

#include "wh/cson/cson.h"
#include "wh/cson/cson_cpdo.h"

#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf


/**
   Converts cson_cpdo "thin" format JSON to "array-of-objects". The
   result is a JSON array contain a list of objects with full
   key/value pair information. The result is structurally identical to
   taking the "rows" field from a "fat"-formatted JSON tree.

   The returned object is newly-created, and must eventually be freed
   by calling cson_value_free() or ownership transfered (e.g. adding
   it to a container).
*/
int cson_cpdo_json_thin_to_aoo( cson_object const * src, cson_value ** tgt )
{
    /* this all does the same thing as the above, actually...
     */
    int rc;
    cson_value * outRowsV = NULL;
    cson_array * outRows = NULL;
    char const * key = NULL;
    cson_array const * cols = NULL;
    cson_array const * rows = NULL;
    cson_array * oldRow = NULL;
    cson_value * newObjV = NULL;
    cson_object * newObj = NULL;
    unsigned int rowsLen;
    unsigned int colsLen;
    unsigned int r, c;
    if( ! src | ! tgt ) return cson_rc.ArgError;
    cols = cson_value_get_array( cson_object_get( src, "columns" ) );
    if( ! cols ) return cson_rc.ArgError;
    rows = cson_value_get_array( cson_object_get( src, "rows" ) );
    if( ! rows ) return cson_rc.ArgError;


    outRowsV = cson_value_new_array();
    if( ! outRowsV ) return cson_rc.AllocError;
    outRows = cson_value_get_array( outRowsV );
    rowsLen = cson_array_length_get( rows );
    colsLen = cson_array_length_get( cols );

    rc = cson_array_reserve( outRows, rowsLen );
    if(rc){
        goto end_clean;
    }
    for( r = 0; r < rowsLen; ++r )
    {
        oldRow = cson_value_get_array( cson_array_get( rows, r ) );
        if( ! oldRow ) continue;
        newObjV = cson_value_new_object();
        if( ! newObjV )
        {
            rc = cson_rc.AllocError;
            goto end_clean;
        }
        rc = cson_array_append( outRows, newObjV );
        if( rc )
        {
            cson_value_free( newObjV );
            goto end_clean;
        }
        newObj = cson_value_get_object( newObjV );
        for( c = 0; c < colsLen; ++c )
        {
            key = cson_string_cstr( cson_value_get_string( cson_array_get( cols, c ) ) );
            if( ! key )
            {
                rc = cson_rc.InternalError;
                goto end_clean;
            }
            rc = cson_object_set( newObj, key, cson_array_get( oldRow, c ) );
            if( rc )
            {
                goto end_clean;
            }
        }
    }
    *tgt = outRowsV;
    return 0;
    end_clean:
    cson_value_free( outRowsV );
    return rc;
}

typedef struct {
    /* If true, use "fat" JSON format, else "thin". */
    char isFat;
    /* Name for use with (-f FMT) argument. */
    char const * name;
    /* Description, for use in help text. */
    char const * descr;
} OutputFormat;

static const OutputFormat OutputFormat_empty = {0,NULL,NULL};
static OutputFormat OutputFormats[] = {
{0,"thin", "As defined by cson_cpdo_sql_to_json()."},
{1,"fat", "As defined by cson_cpdo_sql_to_json()."},
{1,"aoo", "\"Array-of-Objects\", basically the \"rows\" property from \"fat\" format."},
{0,NULL,NULL}
};


struct AppData {
    cpdo_driver * db;
    cson_output_opt outOpt;
    cson_buffer sqlBuf;
    char fat;
    OutputFormat format;
}  ThisApp = {
NULL /* db */,
cson_output_opt_empty_m /*outOpt*/,
cson_buffer_empty_m /*sqlBuf*/,
1 /* fat */,
{1,"fat",0} /*format*/
};

#define DBCHECK(cond,desc) \
    if(!(cond)) do {                                                    \
        char const * dbErrMsg = NULL;                                   \
        ThisApp.db->api->error_info( ThisApp.db, &dbErrMsg, NULL, &rc ); \
        fprintf(stderr,"%s: db error info: code %d=[%s]\n",desc, rc, dbErrMsg); \
        /* reminder: the cpdo_msyql5 driver isn't returning an error for failed statement preparation. */ \
        exit(1);                                                        \
    } while(0)

static void do_select( char const * sql )
{
    cpdo_stmt * st = NULL;
    int rc;
    cson_value * json = NULL;
    rc = cpdo_prepare( ThisApp.db, &st, sql, strlen(sql) );
    DBCHECK((0==rc),"SQL statement preparation failed");
    rc = cson_cpdo_stmt_to_json( st, &json, ThisApp.format.isFat );
    st->api->finalize( st );
    if( rc )
    {
        fprintf(stderr,"Conversion to JSON failed with code %d (%s).\n",
                rc, cson_rc_string(rc));
        exit(rc);
    }
    if( (0==strcmp("aoo",ThisApp.format.name)) )
    {
        cson_value * conv = cson_object_get( cson_value_get_object(json), "rows" );
        if( !conv )
        {
            fprintf(stderr,"Unexpected structure in \"fat\" JSON result set.\n");
            exit(cson_rc.InternalError);
        }
        /*
          What follows, dear cson acolyte, is that we're claiming partial
          ownership of the 'conv' child object, destroying its parent,
          and then overwriting the 'json' pointer with that of the child
          we stole.
         */
        cson_value_add_reference( conv );
        cson_value_free( json );
        json = conv;
    }
    cson_output_FILE( json, stdout, &ThisApp.outOpt );
    cson_value_free( json );
}


static void do_connect( char const * dsn,
                        char const * user,
                        char const * pass )
{
    char const * dbErrMsg = NULL;
    int rc = cpdo_driver_new_connect( &ThisApp.db, dsn, user, pass );
    if( 0 != rc )
    {
        if( ThisApp.db )
        {
            ThisApp.db->api->error_info( ThisApp.db, &dbErrMsg, NULL, &rc );
            fprintf(stderr,"Connect failed: driver-specific code=%d (%s)\n",
                    rc, dbErrMsg);
            ThisApp.db->api->close(ThisApp.db);
            ThisApp.db = NULL;
        }
        else
        {
            fprintf(stderr,"Connect failed: cpdo error code=%d (%s)\n",
                    rc, cpdo_rc_string(rc));
        }
        exit(rc);
    }
}

static void list_drivers()
{
    char const * const * dlist = cpdo_available_drivers();
    for( ; *dlist; ++dlist )
    {
        puts( *dlist );
    }
}


static void show_help(char const * app)
{
    char const * const * dlist = NULL;
    OutputFormat const * fmt = OutputFormats;
    printf("Usage:\n\t%s options\n\n",app);
    /* Did you know that c90 specifies an optional limit on inlined
       string length? i didn't, but gcc says:
       
       error: string length '686' is greater than the length '509' ISO
       C90 compilers are required to support
    */
    puts("The options are:\n"
         "\t[-d DSN] Database driver connection string (cpdo DSN format).\n"
         "\t[-s SQL] The complete, well-formed SELECT query to run.\n"
         "\t[-S SQL_FILENAME] same as -s but reads SQL from a file"
         " (use '-' for stdin)\n"
         "\t[-?|--help] shows this help text and exits with code 0"
         );
    puts("\t[-u USERNAME] (default=$USER)\n"
         "\t[-p PASSWORD]\n"
         "\t[-#] JSON indentation level. 0=no indentation, 1=one TAB per level,"
         " any other number means that many spaces per level.\n"
         "\t[-f FORMAT_NAME] sets output format mode by name. See below. (Default=\"fat\")\n"
         "\t[-t] Alias for [-f thin]\n"
         "\t[-L] lists available db drivers and exits with code 0\n"
         );
    puts("The options -d and one of (-s,-S) are required."
         " -u and -p might be required, depending on the database"
         " driver and configuration.\n"
         "\n"
         "The DSN string format is described in the cpdo wiki:\n\n"
         "\thttp://fossil.wanderinghorse.net/wikis/cpdo/?page=DSN\n"
         );

    puts("Available output formats:\n");
    for( ; fmt->name; ++fmt )
    {
        printf("\t%s: %s\n", fmt->name, fmt->descr );
    }
    putchar('\n');

    
    dlist = cpdo_available_drivers();
    puts("Available drivers:");
    for( ; *dlist; ++dlist )
    {
        putchar('\t');
        puts(*dlist);
    }
    putchar('\n');
}



static void atexit_handler()
{
    cson_buffer_reserve( &ThisApp.sqlBuf, 0 );
    if( ThisApp.db )
    {
        ThisApp.db->api->close( ThisApp.db );
        ThisApp.db = NULL;
    }
}

static void buffer_sql_FILE( FILE * inp )
{
    int const rc = cson_buffer_fill_from( &ThisApp.sqlBuf,
                                          cson_data_source_FILE,
                                          inp );
    if( rc )
    {
        fprintf(stderr,"Error %d (%s) while reading the SQL source file.\n",
                rc, cson_rc_string(rc));
        exit(rc);
    }
}

static void buffer_sql_filename( char const * fn )
{
    FILE * f = NULL;
    assert( NULL != fn );
    if( 0 == strcmp("-",fn) )
    {
         f = stdin;
    }
    else
    {
        f = fopen(fn,"r");
        if( ! f )
        {
            fprintf(stderr,"Could not open file [%s] for reading.\n",
                    fn );
            exit(cson_rc.IOError);
        }
    }
    buffer_sql_FILE( f );
    if( stdin != f ) fclose(f);
}

static void check_format_name( char const * f )
{

    OutputFormat const * pos = OutputFormats;
    for( ; pos->name; ++pos )
    {
        if( 0 == strcmp(pos->name, f) )
        {
            ThisApp.format = *pos;
            return;
        }
    }
    fprintf(stderr,"Unknown output format name [%s]\n", f );
    exit(cson_rc.RangeError);
}

int main( int argc, char const ** argv )
{
    int i = 0;
    char const * dsn = NULL;
    char const * sql = NULL;
    char gotSomething = 0;
    char const * uname = getenv("USER");
    char const * pass = NULL;
    atexit( atexit_handler );
#if CPDO_ENABLE_SQLITE3
    cpdo_driver_sqlite3_register();
#endif
#if CPDO_ENABLE_MYSQL5
    cpdo_driver_mysql5_register();
#endif

    ThisApp.outOpt.indentation = 1;
    ThisApp.outOpt.addNewline = 1;

    for( i = 1; i < argc; ++i )
    {
        char const * arg = argv[i];
        if( '-' == *arg )
        {
            if( (0 == strcmp("-?",arg))
                || (0==strcmp("--help",arg)) )
            {
                show_help(argv[0]);
                return 0;
            }
            else if( 0 == strcmp("-L",arg) )
            {
                list_drivers();
                return 0;
            }
            else if( 0 == strcmp("-u",arg) )
            {
                uname = argv[++i];
                continue;
            }
            else if( 0 == strcmp("-p",arg) )
            {
                pass = argv[++i];
                continue;
            }
            else if( 0 == strcmp("-d",arg) )
            {
                gotSomething = 1;
                dsn = argv[++i];
                continue;
            }
            else if( 0 == strcmp("-s",arg) )
            {
                if( sql )
                {
                    fprintf(stderr,"%s: error: input SQL specified multiple times. "
                            "Use the -s or -S options only ONE time.\n",
                            argv[0]);
                    return cson_rc.ArgError;
                }
                gotSomething = 1;
                sql = argv[++i];
                continue;
            }
            else if( 0 == strcmp("-S",arg) )
            {
                if( sql )
                {
                    fprintf(stderr,"%s: error: input SQL specified multiple times. "
                            "Use the -s or -S options only ONE time.\n",
                            argv[0]);
                    return cson_rc.ArgError;
                }
                gotSomething = 1;
                if( i>= argc-1 )
                {
                    fprintf(stderr,
                            "%s: The -S option requires a filename argument "
                            "(use '-' for stdin).\n", argv[0]);
                    return cson_rc.ArgError;
                }
                buffer_sql_filename( argv[++i] );
                sql = (char const *)ThisApp.sqlBuf.mem;
                continue;
            }
            else if( 0 == strcmp("-t",arg) )
            {
                ThisApp.format = OutputFormat_empty;
                ThisApp.format.name = "thin";
                continue;
            }
            else if( 0 == strcmp("-f",arg) )
            {
                if( (i>= argc-1) )
                {
                    fprintf(stderr,
                            "%s: The -f option requires a format name "
                            "(try one of: thin, fat, aoo).\n", argv[0]);
                    return cson_rc.ArgError;
                }
                check_format_name( argv[++i] );
                continue;
            }
            else if( ('-' == *arg) && (*(arg+1) >= '0') && (*(arg+1)<='9'))
            { /* try -# to set indention level */
                enum { NumBufLen = 16 };
                unsigned short val = 0;
                char buf[NumBufLen];
                int check;
                memset(buf, 0, NumBufLen);
                check = sscanf( arg+1, "%hu", &val );
                if( 1 != check )
                {
                    fprintf(stderr,"%s: Could not parse argument [%s] as a numeric value.\n",
                            argv[0],arg);
                    return 2;
                }
                ThisApp.outOpt.indentation = (unsigned char)val;
                continue;            
            }
            else
            {
                goto unknown_arg;
            }
        }
        unknown_arg:
        fprintf(stderr,"%s: unknown argument [%s]. Try -? or --help.\n",
                argv[0], arg);
        return 1;
    }


    if( !gotSomething )
    {
        show_help(argv[0]);
        return 1;
    }
    if( !dsn )
    {
        fprintf(stderr,"Required -d (DSN) argument is missing.\n");
        return 1;
    }
    if( !sql )
    {
        fprintf(stderr,"Required -s (SQL) argument is missing.\n");
        return 1;
    }

    do_connect( dsn, uname, pass );
    do_select( sql );
    return 0;
}

#undef DBCHECK
#undef MARKER
