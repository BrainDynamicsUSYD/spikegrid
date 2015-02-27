#if !defined(_WIN32) && !defined(_WIN64)
#  if !defined(_POSIX_VERSION)
#    define _POSIX_VERSION 200112L /* chmod(), unlink() */
#  endif
#  define ENABLE_POSIX_FILE_OPS 1
#else
#  define ENABLE_POSIX_FILE_OPS 0
#endif

#include "wh/cson/cson_session.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#if ENABLE_POSIX_FILE_OPS
#  define UNLINK_FILE unlink
#  include <unistd.h> /* unlink() */
#  include <sys/stat.h> /* chmod() */
#else
/* http://msdn.microsoft.com/en-us/library/1c3tczd6(v=vs.80).aspx
  #  define UNLINK_FILE _unlink
  #  include <io.h>
*/
#  define UNLINK_FILE remove
#  include <stdio.h> /* remove(), _unlink() */
#endif

static int cson_session_file_load( cson_sessmgr * self, cson_value ** tgt, char const * id );
static int cson_session_file_save( cson_sessmgr * self, cson_value const * root, char const * id );
static int cson_session_file_remove( cson_sessmgr * self, char const * id );
static void cson_session_file_finalize( cson_sessmgr * self );

static const cson_sessmgr_api cson_sessmgr_api_file =
{
    cson_session_file_load,
    cson_session_file_save,
    cson_session_file_remove,
    cson_session_file_finalize
};

typedef struct cson_sessmgr_file_impl cson_sessmgr_file_impl;
struct cson_sessmgr_file_impl
{
    char * dir;
    char * prefix;
    char * suffix;
};

static const cson_sessmgr cson_sessmgr_file_empty =
{
    &cson_sessmgr_api_file,
    NULL
};

static char * cson_session_file_strdup( char const * src )
{
    size_t const n = src ? strlen(src) : 0;
    char * rc = src ? (char *)calloc(1, n+1) : NULL;
    if( ! rc ) return NULL;
    memcpy( rc, src, n );
    return rc;
}

/* Helper macro for varios cson_sessmgr_api member implementations. */
#define IMPL_DECL(RC) \
    cson_sessmgr_file_impl * impl = (self && (self->api == &cson_sessmgr_api_file)) \
    ? (cson_sessmgr_file_impl*)self->impl \
    : NULL;                                   \
    if( NULL == impl ) return RC

static int cson_session_file_name( cson_sessmgr_file_impl * impl,
                                   char const * id,
                                   char * buf, unsigned int bufLen )
{
    char const * dir = impl->dir ? impl->dir : ".";
    char const * pre = impl->prefix ? impl->prefix : "";
    char const * suf = impl->suffix ? impl->suffix : "";
    char * pos = NULL /* current write possition. */;
    unsigned int flen = 0 /* length of the next token. */;
    unsigned int olen = 0 /* total number of bytes written so far. */;
    if( ! id || !*id ) return cson_rc.ArgError;

#define CHECKLEN if(olen >= bufLen) return cson_rc.RangeError; assert( pos < (buf+bufLen) )
    pos = buf;

#define PUSH(FIELD) \
    flen = strlen(FIELD); \
    olen += flen; \
    CHECKLEN; \
    strncpy( pos, FIELD, flen ); \
    pos += flen

    PUSH(dir);

    ++olen;
    CHECKLEN;
#if defined(_WIN32)
    *(pos++) = '\\';
#else
    *(pos++) = '/';
#endif

    PUSH(pre);
    PUSH(id);
    PUSH(suf);
    if( pos >= (buf + bufLen) ) return cson_rc.RangeError;
    *pos = 0;
    return 0;
#undef PUSH
#undef CHECKLEN
}

static int cson_session_file_load( cson_sessmgr * self, cson_value ** root, char const * id )
{
    enum { BufSize = 1024 };
    char fname[BufSize];
    FILE * fh = NULL;
    int rc;
    IMPL_DECL(cson_rc.ArgError);
    if( !root || !id || !*id ) return cson_rc.ArgError;
    memset( fname, 0, BufSize );
    rc = cson_session_file_name( impl, id, fname, BufSize );
    if( 0 != rc ) return rc;
    fh = fopen( fname, "r" );
    if( ! fh ) return cson_rc.IOError;
    rc = cson_parse_FILE( root, fh, NULL, NULL );
    fclose( fh );
    return rc;
}

static int cson_session_file_save( cson_sessmgr * self, cson_value const * root, char const * id )
{
    enum { BufSize = 1024 };
    char fname[BufSize];
    FILE * fh = NULL;
    int rc;
    IMPL_DECL(cson_rc.ArgError);
    if( !root || !id || !*id ) return cson_rc.ArgError;
    memset( fname, 0, BufSize );

    rc = cson_session_file_name( impl, id, fname, BufSize );
    if( 0 != rc ) return rc;
    fh = fopen( fname, "w" );
    if( ! fh ) return cson_rc.IOError;
#if ENABLE_POSIX_FILE_OPS
    chmod( fname, 0600 );
#endif
    rc = cson_output_FILE( root, fh, NULL );
    fclose( fh );
    if( rc )
    {
        UNLINK_FILE( fname );
    }
    return rc;
}

void cson_session_file_finalize( cson_sessmgr * self )
{
    if( self && (self->api == &cson_sessmgr_api_file) )
    {
        cson_sessmgr_file_impl * impl = (cson_sessmgr_file_impl *)self->impl;
        free( impl->dir );
        free( impl->prefix );
        free( impl->suffix );
        free( impl );
        *self = cson_sessmgr_file_empty;
        free( self );
    }
}

static int cson_session_file_remove( cson_sessmgr * self, char const * id )
{
    enum { BufSize = 1024 };
    char fname[BufSize];
    int rc;
    IMPL_DECL(cson_rc.ArgError);
    if( !id || !*id ) return cson_rc.ArgError;
    memset( fname, 0, BufSize );
    rc = cson_session_file_name( impl, id, fname, BufSize );
    if( 0 != rc ) return rc;
    rc = UNLINK_FILE( fname );
    return (0==rc) ? 0 : cson_rc.IOError;
}


int cson_sessmgr_file( cson_sessmgr ** tgt, cson_object const * opt )
{
    int rc;
    cson_sessmgr * m = tgt ? (cson_sessmgr *)malloc(sizeof(cson_sessmgr)) : NULL;
    cson_sessmgr_file_impl * impl = m
        ? (cson_sessmgr_file_impl *)malloc(sizeof(cson_sessmgr_file_impl))
        : NULL;
    if( ! m ) return tgt ? cson_rc.AllocError : cson_rc.ArgError;
    else if( ! impl )
    {
        free(m);
        return cson_rc.AllocError;
    }
    *m = cson_sessmgr_file_empty;
    m->impl = impl;
    if( opt )
    {
        cson_string const * jstr;
#define CP(KEY) \
        jstr = cson_value_get_string( cson_object_get( opt, # KEY ) ); \
        if( jstr ) { \
            impl->KEY = cson_session_file_strdup( cson_string_cstr( jstr ) ); \
            if( ! impl->KEY ) { \
                rc = cson_rc.AllocError;        \
                goto error_clean;               \
            } \
        } (void)0
        
        CP(dir);
        CP(prefix);
        CP(suffix);
#undef CP
    }
#define CP(KEY,VAL) if( ! impl->KEY ) { \
            impl->KEY = cson_session_file_strdup(VAL); \
            if( ! impl->KEY ) { \
                rc = cson_rc.AllocError;        \
                goto error_clean;               \
            } \
        } (void)0
#if ENABLE_POSIX_FILE_OPS
    CP(dir,"/tmp");
#else
    CP(dir,".");
#endif
    CP(prefix,"cson-session-");
    CP(suffix,".json");
#undef CP
    *tgt = m;
    return 0;
    error_clean:
    m->api->finalize( m );
    return rc;
}

#undef IMPL_DECL
#undef ENABLE_POSIX_FILE_OPS
#undef UNLINK_FILE
