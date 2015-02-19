
/************************************************************************
FAR FROM COMPLETE.

This is a work-in-progress

Author: Stephan Beal (http://wanderinghorse.net/home/stephan/)

License: Public Domain
************************************************************************/

#if defined(NDEBUG)
#  undef NDEBUG
#endif
#include <assert.h>

#include "wh/whio/whio_amalgamation.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // memset() and friends
#include <ctype.h> /* isblank() */
#include <unistd.h> /* isatty() */


static int my_printfv(char const * fmt, va_list vargs )
{
    return vfprintf( stderr, fmt, vargs );
}
static int my_printf(char const * fmt, ...)
{
    if(0) my_printf(0);
    va_list vargs;
    int rc;
    va_start(vargs,fmt);
    rc = my_printfv( fmt, vargs );
    va_end(vargs);
    return rc;
}
#define MARKER(mp_exp) do { my_printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); my_printf mp_exp; my_printf("\n");} while(0)


#include "AppCommand.c"

typedef struct Buffer
{
    char * mem;
    size_t alloced;
} Buffer;
#define Buffer_empty_m {NULL,0}
static const Buffer Buffer_empty = Buffer_empty_m;

struct Application
{
    char const * file;
    whio_ht * ht;
    char const * funcSet;
    whio_iomodes iomode;
    whio_dev * dev;
    bool insertAllowsReplace;
    bool searchFailIfNotFound;
    bool delFailIfNotFound;
    bool createCanOverwrite;
    Buffer keyBuf;
    Buffer valBuf;
    whio_ht_opt htopt;
} ThisApp = {
NULL/*file*/,
NULL/*ht*/,
//"string:nocase"/*funcSet*/,
"string"/*funcSet*/,
WHIO_MODE_UNKNOWN/*iomode*/,
NULL/*dev*/,
false/*insertAllowsReplace*/,
true/*searchFailIfNotFound*/,
true/*delFailIfNotFound*/,
false/*createCanOverwrite*/,
Buffer_empty_m/*keyBuf*/,
Buffer_empty_m/*valBuf*/,
{/*htopt*/
769/*hashSize*/,
/** useful primes: http://en.wikipedia.org/wiki/List_of_prime_numbers
53, 97, 193, 389,
769, 1543, 3079, 6151,
12289, 24593, 49157, 98317,
196613, 393241, 786433, 1572869,
*/
whio_mutex_empty_m
}
};


static void ht_check_rc(int rc)
{
    if( rc )
    {
        MARKER(("TAKE NOTE: EXITING WITH ErRoR cOdE %d/%s",rc,whio_rc_string(rc)));
        exit(rc);
    }
}
static int buffer_reserve( Buffer * b, whio_size_t n )
{
    if( n == 0 )
    {
        free(b->mem);
        *b = Buffer_empty;
        return 0;
    }
    else if( n < b->alloced )
    {
        return 0;
    }
    else
    {
        ++n /* NULL pad */;
        char * x = realloc( b->mem, n );
        if( ! x ) return whio_rc.AllocError;
        b->mem = x;
        b->alloced = n;
        return 0;
    }
}

static void buffers_reserve( size_t keyLen, size_t valLen )
{
    int rc = buffer_reserve( &ThisApp.keyBuf, keyLen );
    ht_check_rc( rc );
    rc = buffer_reserve( &ThisApp.valBuf, valLen );
    ht_check_rc( rc );
    memset( ThisApp.keyBuf.mem, 0, ThisApp.keyBuf.alloced );
    memset( ThisApp.valBuf.mem, 0, ThisApp.valBuf.alloced );

}

static void buffers_fill( whio_ht_record const * rec )
{
    size_t const ksz = whio_ht_key_len( rec );
    size_t const vsz = whio_ht_value_len( rec );
    buffers_reserve( ksz, vsz );
    int rc = whio_ht_key_get( ThisApp.ht, rec, ThisApp.keyBuf.mem );
    ht_check_rc(rc);
    rc = whio_ht_value_get( ThisApp.ht, rec, ThisApp.valBuf.mem );
    ht_check_rc(rc);
}

int ht_open_dev( bool allowCreate, bool allowExisting )
{
    if( ThisApp.dev )
    {
        MARKER(("USAGE ERROR: device is already opened!"));
        return whio_rc.ArgError;
    }
    char const * fn = ThisApp.file;
    if( ! fn || !*fn )
    {
        MARKER(("ERROR: no whio_ht filename specified!"));
        return whio_rc.RangeError;
    }
    whio_dev * dev = NULL;
    bool created = false;
    // FIXME: use whio_dev_posix_open2() instead of this crap...
    dev = whio_dev_for_filename( fn, "r+" );
    if( ! allowExisting && dev )
    {
        MARKER(("File [%s] already exists, and i'm in non-hammer mode!",fn));
        return whio_rc.AccessError;
    }
    if( allowCreate && !dev && (ThisApp.iomode>0))
    {
        created = true;
        dev = whio_dev_for_filename( fn, "w+" );
    }
    if( ! dev )
    {
        MARKER(("ERROR: could not open db [%s]!",fn));
    }
    else
    {
#if 0 /* FIXME: verify that dev is also a whio_ht*/
        if( !created && is-a-ht )
        {
            dev->api->finalize(dev);
            dev = NULL;
            MARKER(("ERROR: [%s] is not a whio_ht!",fn));
            return whio_rc.TypeError;
        }
#endif
        //MARKER(("Opened whio_ht file [%s].",fn));
    }
    ThisApp.dev = dev;
    return dev ? 0 : whio_rc.IOError;
}

int ht_open()
{
    if( ThisApp.ht )
    {
        MARKER(("USAGE ERROR: ht is already opened!"));
        return whio_rc.AccessError;
    }
    whio_dev * dev = ThisApp.dev;
    int rc = 0;
    if( ! dev ) rc = ht_open_dev( false, true );
    if( rc )
    {
        MARKER(("open failed for file [%s]!",ThisApp.file));
        return rc;
    }
    dev = ThisApp.dev;
    if( ! dev )
    {
        MARKER(("USAGE ERROR: dev is NULL!"));
        return whio_rc.ArgError;
    }
    rc = whio_ht_open( &ThisApp.ht, dev );
    if( rc )
    {
        MARKER(("ERROR: whio_ht_open() failed for db [%s] with code [%s]!",
                ThisApp.file, whio_rc_string(rc)));
        dev->api->finalize(dev);
    }
    return rc;
}

int ht_format()
{
    int rc = whio_rc.NYIError;
    if( ThisApp.ht )
    {
        MARKER(("USAGE ERROR: ht is already opened!"));
        return whio_rc.AccessError;
    }
    whio_ht_funcset fn = whio_ht_funcset_empty;
    rc = whio_ht_funcset_parse( ThisApp.funcSet, &fn );
    if( rc )
    {
        MARKER(("ERROR: function set name [%s] is invalid. Error code=%d/%s!",
                ThisApp.funcSet, rc, whio_rc_string(rc) ));
        return rc;
    }
    rc = ht_open_dev( true, ThisApp.createCanOverwrite );
    if( rc ) return rc;
    ThisApp.dev->api->truncate( ThisApp.dev, 0 );
    rc = whio_ht_format( &ThisApp.ht, ThisApp.dev, &ThisApp.htopt,
                         ThisApp.funcSet );
    if( rc )
    {
        MARKER(("ERROR: format failed with rc %d/%s!",rc,whio_rc_string(rc)));
    }
    else
    {
        printf("formatted file [%s] as a whio_ht.\n",ThisApp.file);
    }
    putchar('\n');
    return rc;
}

/**
   Reads line from stdin. On success:

   *key == key part of line (first token)

   *val = val part, or NULL.

   *breakOut == non-0 if EOF was reached and reading
   should stop.

   On error, returns non-0 and the key/val args are not modified.
*/
int ht_gets_stdin( char ** key, char ** val, bool * breakOut )
{
    enum { LineLen = 256 };
    static char buf[LineLen] = {0};
    static char kbuf[LineLen] = {0};
    static char dbuf[LineLen] = {0};
    int rc = 0;
    *breakOut = false;
    do
    {
        char * r = fgets( buf, LineLen, stdin );
        if( ! r )
        {
            *breakOut = true;
            break;
        }
        else if(  (*r=='\n') ) continue;
        char * at = r;
        for( ; *at && !isspace(*at); ++at )
        {
        }
        if( *at == '\n' )
        {
            *at = 0;
        }
        memset( kbuf, 0, LineLen );
        memset( dbuf, 0, LineLen );
        memcpy( kbuf, r, (at-r) );
        for( ; *at && (isblank(*at)); ++at)
        {
            *at = 0;
        }
        r = at;
        for( ; *at && ('\n'!=*at); ++at )
        {
        }
        memcpy( dbuf, r, (at-r) );
        if( 0 && ! *dbuf )
        {
            memcpy( dbuf, kbuf, LineLen );
        }
    }while(false);
    if( 0 == rc )
    {
        *key = kbuf;
        *val = dbuf;
    }
    else
    {
        *key = NULL;
        *val = NULL;
    }
            
    return rc;
   
}

int ht_insert()
{
    if( ! ThisApp.ht )
    {
        MARKER(("ERROR: ht is not open!"));
        return whio_rc.ArgError;
    }
    int rc = 0;
    if( isatty(0) )
    {
        puts("Inputing from stdin. Format is: KEY [OPTIONAL VALUE UNTIL EOL]");
    }
    do
    {
        char * k = NULL;
        char * v = NULL;
        bool breakOut = false;
        rc = ht_gets_stdin( &k, &v, &breakOut );
        if( rc ) break;
        else if( breakOut ) break;
        else if( ! k || !*k ) continue;
        rc =whio_ht_insert( ThisApp.ht, k, strlen(k), v, strlen(v) );
        // FIXME: support: ThisApp.insertAllowsReplace
        if( whio_rc.AccessError == rc )
        {
            printf("Insert _apparently_ failed due to key collision: [%s]\n",k);
            break;
        }
        else if( rc ) break;
        if(1)
        {
            printf("Inserted%s: [%s]=[%s]\n",
                   (ThisApp.insertAllowsReplace ? "/Replaced" : ""),
                   k, v);
        }
    } while( !rc );
    if( rc )
    {
        MARKER(("Error code=%d/%s", rc, whio_rc_string(rc) ));
    }
    return rc;
}

int ht_remove( int argc, char const * const* argv)
{
    if( argc < 1 )
    {
        MARKER(("USAGE ERROR: remove requires key names as arguments."));
        return whio_rc.ArgError;
    }
    int i = 0;
    char const * arg = NULL;
    int rc = 0;
    for( ; i < argc; ++i )
    {
        arg = argv[i];
        //MARKER(("key to remove=[%s]",arg));
        //printf("\tr@%p [%s]=[%s]",(void const *)r,r->key,r->data);
        printf("\tRemoving [%s] ...",arg);
        rc = whio_ht_remove( ThisApp.ht, arg, strlen(arg) );
        if( whio_rc.NotFoundError == rc )
        {
            printf("\tNOT FOUND! ");
            if( ThisApp.delFailIfNotFound )
            {
                puts("Giving up!");
                break;
            }
            else
            {
                puts("Ignoring.");
                rc = 0;
                continue;
            }
        }
        else if( rc ) break;
        printf("\tREMOVED\n");
    }
    return rc;
}

int ht_search( int argc, char const * const* argv)
{
    if( argc < 1 )
    {
        MARKER(("USAGE ERROR: search requires key names as arguments."));
        return whio_rc.ArgError;
    }
    int i = 0;
    char const * arg = NULL;
    int rc = 0;
    whio_ht_record rec = whio_ht_record_empty;
    for( ; i < argc; ++i )
    {
        arg = argv[i];
        rec = whio_ht_record_empty;
        rc = whio_ht_search(ThisApp.ht, arg, strlen(arg), &rec);
        if( whio_rc.NotFoundError == rc )
        {
            printf("NOT FOUND: [%s]\n",arg);
            if( ThisApp.searchFailIfNotFound )
            {
                return rc;
            }
            continue;
        }
        else if( 0 != rc )
        {
            return rc;
        }
        buffers_fill( &rec );
        printf("[%s]=[%s]\n",ThisApp.keyBuf.mem, ThisApp.valBuf.mem);
    }
    if( rc )
    {
        MARKER(("Error code=%d/%s", rc, whio_rc_string(rc) ));
    }
    return rc;
}

int ht_ls()
{
    if( ! ThisApp.ht )
    {
        MARKER(("ERROR: ht is not open!"));
        return whio_rc.ArgError;
    }
    whio_ht_iterator iter = whio_ht_iterator_empty;
    int rc = whio_ht_iterator_begin( ThisApp.ht, &iter );
    assert( 0 == rc );
    whio_size_t count = 0;
    while( ! whio_ht_iterator_is_end( &iter ) )
    {
        ++count;
        buffers_fill( &iter.record );
        printf("[%"WHIO_SIZE_T_PFMT"]=[%s]=[%s]\n",
               iter.record.block.id,
               ThisApp.keyBuf.mem, ThisApp.valBuf.mem);
        rc = whio_ht_iterator_next( &iter );
        if( rc ) return rc;
    }
    return 0;    
}


static void ht_open_or_die()
{
    int rc = ht_open();
    ht_check_rc(rc);
}

int cmd_create_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
    int rc = 0;
    if( argc )
    {
        struct SizeParse
        {
            char const * key;
            whio_size_t * sz;
        } szargs[] =
              {
              {"hashSize",&ThisApp.htopt.hashSize},
              {NULL,NULL}
              };
        size_t i = 0;
        bool doCont = false;
        for( ; i < argc; ++i )
        {
            char const * arg = argv[i];
            doCont = false;
            if( 0 == strcmp("-f",arg) )
            {
                ThisApp.createCanOverwrite = true;
                continue;
            }
            else if( 0 == strcmp("funcSet",arg) )
            {
                ++i;
                if( i == argc )
                {
                    MARKER(("'funcSet' requires a string argument"));
                    return whio_rc.ArgError;
                }
                arg = argv[i];
                ThisApp.funcSet = arg;
                continue;
            }
            struct SizeParse * szp = szargs;
            for( ; szp->key; ++szp )
            {
                if( 0 == strcmp(szp->key,arg) )
                {
                    ++i;
                    if( i == argc )
                    {
                        MARKER(("'%s' requires a numeric argument",szp->key));
                        return whio_rc.ArgError;
                    }
                    arg = argv[i];
                    if( 1 != sscanf( arg, "%"WHIO_SIZE_T_SFMT, szp->sz ) )
                    {
                        MARKER(("Parse of %s from [%s] failed!",szp->key,argv));
                        return whio_rc.ArgError;
                    }
                    doCont = true;
                }
            }
            if( doCont ) continue;                
            MARKER(("Unknown flag: [%s]",argv[i]));
            rc = whio_rc.ArgError;
            ht_check_rc(rc); // will not return
        }
    }
    ThisApp.iomode = WHIO_MODE_RW;
    rc = ht_format();
    ht_check_rc(rc);
    return AppCommandOK;
}

int cmd_insert_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
    ht_open_or_die();
    int rc = ht_insert();
    ht_check_rc(rc);
    return AppCommandOK;
}

int cmd_insertReplace_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
#if 0
    ThisApp.insertAllowsReplace = true;
    return cmd_insert_f( self, runData, argc, argv );
#else
    return whio_rc.NYIError;
#endif

}

int cmd_rm_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
    ht_open_or_die();
    int rc = 0;
    size_t i = 0;
    if( argc )
    {
        for( ; i < argc; ++i )
        {
            char const * arg = argv[i];
            if( 0 == strcmp("-f",arg) )
            {
                ThisApp.delFailIfNotFound = false;
                continue;
            }
            break;
        }
    }
    rc = ht_remove( (int)argc-i, argv+i );
    ht_check_rc(rc);
    return AppCommandOK;
}
int cmd_rmF_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
    ThisApp.delFailIfNotFound = false;
    return cmd_rm_f( self, runData, argc, argv );
}
int cmd_search_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
    ht_open_or_die();
    int rc = ht_search( (int)argc, argv );
    ht_check_rc(rc);
    return AppCommandOK;
}

int cmd_ls_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
    ht_open_or_die();
    int rc = ht_ls();
    ht_check_rc(rc);
    return AppCommandOK;
}

int cmd_magic_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
#if 0
    udb_open_or_die();
    int rc = udb_magic();
    udb_check_rc(rc);
    return AppCommandOK;
#else
    return whio_rc.NYIError;
#endif
}

int cmd_dummy_f( AppCommand const * self, void const * runData, size_t argc, char const * const * argv )
{
    MARKER(("cmd_dummy_f([%s],%u,(array)) runData=[%s]", self->name, argc, (char const *)runData ));
    return AppCommandOK;
}



static const AppCommand cmds_main[] =
    {
    { "create",
      "Formats a file as whio_ht storage. Use -f to force overwrite of existing files.",
      "[-f] [hashSize #] [funcSet string]",
      cmd_create_f, "create->runData!", 0, NULL
    },
    { "insert",
      "Reads in keys from stdin. First token per line is the key, the rest is the value.",
      "",
      cmd_insert_f, "insert->runData!", 0, NULL
    },
    { "i",
      "Alias for 'insert'.",
      "",
      cmd_insert_f, "i->runData!", 0, NULL
    },
    { "ls",
      "Lists all entries in the hashtable.",
      "",
      cmd_ls_f, "ls->runData!", 0, NULL
    },
#if 0
    { "magic",
      "Lists internal/magic numbers and such.",
      "",
      cmd_magic_f, "magic->runData!", 0, NULL
    },
    { "replace",
      "Like 'insert' but allows replacing of records.",
      "",
      cmd_insertReplace_f, "replace->runData!", 0, NULL
    },
#endif
    { "rm",
      "Delete a list of keys.",
      "key_1 [... key_n]",
      cmd_rm_f, "rm->runData!", 1, NULL
    },
    { "RM",
      "Like 'rm' but does not fail if an item is not found.",
      "key_1 [... key_n]",
      cmd_rmF_f, "rmF->runData!", 1, NULL
    },
    { "search",
      "search for a list of keys.",
      "key1 [... key_n]",
      cmd_search_f, "search->runData!", 1, NULL
    },
    { "s",
      "Alias for 'search'",
      "key1 [... key_n]",
      cmd_search_f, "s->runData!", 1, NULL
    },
    AppCommand_empty_m
    };

static AppCommand cmd_main =
    {"whio-ht-tool",
     "A tool for working with whio_udb data files.",
     "WHIO_HT_FILE [-m] SUBCOMMAND [subcommand [options]]",
     AppCommand_try_subcommands_f, NULL, 0, cmds_main
    };


void my_atexit()
{
    buffers_reserve( 0, 0 );
    if( ThisApp.ht )
    {
        whio_ht_close( ThisApp.ht );
        ThisApp.ht  = NULL;
    }
    else if( ThisApp.dev )
    {
        ThisApp.dev->api->finalize(ThisApp.dev);
    }
    ThisApp.dev = NULL;
}

void my_help()
{
    AppCommand_show_help( &cmd_main, 0 );
}

int main(int argc, char const * *argv)
{
    atexit( my_atexit );
    int rc = 0;
    int cmdArgOffset = 1;
    cmd_main.name = argv[0];
    if( argc < 2 )
    {
        bail_with_help:
        my_help();
        rc = whio_rc.ArgError;
    }
    else
    {
        unsigned int i = 1;
        if( '-' == *argv[i] )
        {
            char const * arg = NULL;
            for( ; (i < argc) && ('-'==*argv[i]); ++i )
            {
                arg = argv[i];
                MARKER(("Unknown flag: [%s]",arg));
                ht_check_rc( whio_rc.ArgError ); // will not return;
            }
        }

        if( i >= argc )
        {
            goto bail_with_help;
        }

        ++cmdArgOffset;
        ThisApp.file = argv[i];
        rc = AppCommand_try_subcommands_f( &cmd_main, "Hi, world!",
                                           (size_t)argc-cmdArgOffset,
                                           argv+cmdArgOffset );
    }
    ht_check_rc(rc); // will not return if (rc!=0)
    return rc;
}
