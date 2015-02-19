#include <assert.h>
#include <stdlib.h> /* environ, getenv(), atexit() */
#include <ctype.h> /* isspace() */
#include <string.h> /* strlen() */
#include <stdarg.h>
#include <time.h>
#include <locale.h> /* setlocale(), needed for JSON parser. */
#include "wh/cson/cson_cgi.h"
#include "wh/cson/cson_session.h"
#include "whuuid.c"

#if CSON_ENABLE_UNIX
#  define CSON_CGI_USE_SIGNALS 1
#else
#  define CSON_CGI_USE_SIGNALS 0
#endif

/* If RNG_FILENAME evaluates to true then we use that file for getting
   random bytes for session IDs. FIXME: we effectively leak a file
   handle if this is enabled.
*/
#if 0
#  define RNG_FILENAME "/dev/urandom"
#else
#  define RNG_FILENAME NULL
#endif


#if 1
#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf
#else
static void noop_printf(char const * fmt, ...) {}
#define MARKER if(0) printf
#endif

#if CSON_CGI_USE_SIGNALS
#  include <signal.h> /* signal() */
#endif

const cson_cgi_init_opt cson_cgi_init_opt_empty = cson_cgi_init_opt_empty_m;

/**
   Some cson_cgi-internal value keys.
*/
static const struct {
    char const * ENV_GET;
    char const * ENV_POST;
    char const * ENV_COOKIE;
    char const * ENV_SYS;
    char const * ENV_APP;
    char const * ENV_ARGV;
    char const * ENV_CONFIG;
    char const * ENV_SESSION;
    char const * RESPONSE_HEADERS;
} cson_cgi_keys = {
"$GET",
"$POST",
"$COOKIE",
"$ENV",
"$APP",
"$ARGV",
"$CONFIG",
"$SESSION",
"response.headers"
};


/**
   Shared state used by the cson_cgi API.
*/
const cson_cgi_cx cson_cgi_cx_empty = cson_cgi_cx_empty_m;

static int cson_cgi_printf(cson_cgi_cx * cx, char const * fmt, ... )
{
    if( ! fmt ) return 0;
    else
    {
        int rc;
        va_list vargs;
        assert( NULL != cx->opt.outStream );
        va_start( vargs, fmt );
        rc = vfprintf( cx->opt.outStream, fmt, vargs );
        /*if( rc > 0 ) fflush( cx->opt.outStream );*/
        va_end( vargs );
        return rc;
    }
}

static int cson_cgi_puts(cson_cgi_cx * cx, char const * str)
{
    size_t const slen = str ? strlen(str) : 0;
    if( slen )
    {
        if( 1 != fwrite( str, slen, 1, cx->opt.outStream ) )
        {
            return -1;
        }
    }
    if( 1 != fwrite( "\n", 1, 1, cx->opt.outStream ) )
    {
        return -2;
    }
    return (int) (slen + 1);
}

static int cson_cgi_putchar(cson_cgi_cx * cx, char ch)
{
    return ( 1 == fwrite( &ch, 1, 1, cx->opt.outStream ) )
        ? 1
        : -1;
}


cson_value * cson_cgi_argv(cson_cgi_cx *cx)
{
    return cx ? cx->argv.jval : NULL;
}

cson_array * cson_cgi_argv_array(cson_cgi_cx * cx)
{
    return cx ? cson_value_get_array( cx->argv.jval ) : NULL;
}

int cson_cgi_gc_add( cson_cgi_cx * cx, char const * key, cson_value * v, char freeOnError )
{
    int const rc = cson_object_set( cx->gc.jobj, key, v );
    if( (0 != rc) && freeOnError )
    {
        cson_value_free( v );
    }
    return rc;
}

int cson_cgi_response_root_set( cson_cgi_cx * cx, cson_value * v )
{
    if( ! cx ) return cson_rc.ArgError;
    else if( v && !cson_value_is_object(v) && !cson_value_is_array(v) )
    {
        return cson_rc.TypeError;
    }
    else if( cx->response.root != v )
    {
        int rc = 0;
        rc = cson_cgi_gc_add(cx, "response.root", v, 0 )
            /** TODO: confirm that cson_object_set() does not
                clean up the original object if insertion fails.
                If it does, we've just hosed the root node.
            */
            ;
        if( 0 != rc )
        {
            return rc;
        }
        else
        {
            cx->response.root = v;
            return 0;
        }
    }
    else
    {
        return 0;
    }
    
}
cson_value * cson_cgi_response_root_get( cson_cgi_cx * cx, char createMode )
{
    if( ! cx ) return NULL;
    else if( cx->response.root ) return cx->response.root;
    else
    {
        if( 0 != createMode )
        {
            if( createMode > 0 )
            {
                cx->response.root = cson_value_new_object();
            }
            else if( createMode < 0 )
            {
                cx->response.root = cson_value_new_array();
            }
            if( cx->response.root &&
                (0 != cson_cgi_gc_add(cx, "response.root", cx->response.root, 1 )) )
            {
                cx->response.root = NULL /* was cleaned up by cson_cgi_gc_add() */;
            }
        }
        return cx->response.root;
    }
}


/** @internal

Tokenizes an input string on a given separator. Inputs are:

- (inp) = is a pointer to the pointer to the start of the input.

- (separator) = the separator character

- (end) = a pointer to NULL. i.e. (*end == NULL)

This function scans *inp for the given separator char or a NULL char.
Successive separators at the start of *inp are skipped. The effect is
that, when this function is called in a loop, all neighboring
separators are ignored. e.g. the string "aa.bb...cc" will tokenize to
the list (aa,bb,cc) if the separator is '.' and to (aa.,...cc) if the
separator is 'b'.

Returns 0 (false) if it finds no token, else non-0 (true).

Output:

- (*inp) will be set to the first character of the next token.

- (*end) will point to the one-past-the-end point of the token.

If (*inp == *end) then the end of the string has been reached
without finding a token.

Post-conditions:

- (*end == *inp) if no token is found.

- (*end > *inp) if a token is found.

It is intolerant of NULL values for (inp, end), and will assert() in
debug builds if passed NULL as either parameter.

When looping, one must be sure to re-set the inp and end
parameters. For example:

@code
char const * head = input;
char const * tail = NULL;
while( cson_cgi_next_token( &inp, '/', &tail ) ) {
  ...
  head = tail;
  tail = NULL;
}
@endcode

If the loop calls 'continue', it must be careful to
ensure that the parameters are re-set, to avoid an endless
loop. This can be simplified with a goto:

@code
while( cson_cgi_next_token( &inp, '/', &tail ) ) {
  if( some condition ) {
     ... do something ...
     goto next_iter;
  }
  else {
    ....
  }
  next_iter;
  head = tail;
  tail = NULL;
}
@endcode

*/
char cson_cgi_next_token( char const ** inp, char separator, char const ** end )
{
    char const * pos = NULL;
    assert( inp && end && *inp );
    if( ! inp || !end ) return 0;
    else if( *inp == *end ) return 0;
    pos = *inp;
    if( !*pos )
    {
        *end = pos;
        return 0;
    }
    for( ; *pos && (*pos == separator); ++pos) { /* skip preceeding splitters */ }
    *inp = pos;
    for( ; *pos && (*pos != separator); ++pos) { /* find next splitter */ }
    *end = pos;
    return (pos > *inp) ? 1 : 0;
}

/**
   If map->jval is NULL then map->jval is created using
   cson_value_new_object() and map->jobj is assigned to its object
   reference. The newly-created map->jval is appended to
   cx->gc to ensure that map->jval lives a full life (as
   opposed to potentially being prematurly GC'd if a client later adds
   map->jval to his own container).

   If map->jval is not NULL then this function is a no-op.

   This function will assert() if map is NULL.

   Returns 0 on success, else cson_rc.AllocError. On error map->jval
   will be NULL after this call.

   On success, ownership of map->jval is transfered to (or potentially
   shared with) cx->gc.
*/
static int cson_cgi_init_env_map( cson_cgi_cx * cx, char const * gckey, cson_cgi_env_map * map )
{
    int rc = 0;
    assert( NULL != map );
    if( NULL == map->jval )
    {
        assert( NULL == map->jobj );
        map->jval = cson_value_new_object();
        if( NULL == map->jval ) return cson_rc.AllocError;
        rc = cson_cgi_gc_add( cx, gckey, map->jval, 1 )
            /* We do this to avoid a corner case in cleanup logic
               if the client stores this object in another container.
            */;
        if( 0 != rc )
        {
            map->jval = NULL /* was cleaned up by cson_cgi_gc_add() */;
        }
        else
        {
            map->jobj = cson_value_get_object( map->jval );
            assert( NULL != map->jobj );
        }
    }
    return rc;
}

char const * cson_cgi_getenv_cstr( cson_cgi_cx * cx, char const * where, char const * key )
{
    return cson_string_cstr( cson_value_get_string( cson_cgi_getenv(cx, where, key) ) );
}

cson_value * cson_cgi_path_part( cson_cgi_cx * cx, unsigned short ndx )
{
    cson_value * piV = cson_cgi_getenv( cx, "e", "PATH_INFO_SPLIT" );
    if( ! piV ) return NULL;
    else
    {
        unsigned int alen;
        cson_array * ar = cson_value_get_array(piV);
        assert( NULL != ar );
        alen = cson_array_length_get( ar );
        return ( ndx >= alen )
            ? NULL
            : cson_array_get( ar, ndx );
    }
}

char const * cson_cgi_path_part_cstr( cson_cgi_cx * cx, unsigned short ndx )
{
    return cson_string_cstr( cson_value_get_string( cson_cgi_path_part( cx, ndx ) ) );
}

/**
   cson_cgi_hexchar_to_int():

   For 'a'-'f', 'A'-'F' and '0'-'9', returns the appropriate decimal
   number.  For any other character it returns -1.
*/
static int cson_cgi_hexchar_to_int( int ch )
{
    if( (ch>='a' && ch<='f') ) return ch-'a'+10;
    else if( (ch>='A' && ch<='F') ) return ch-'A'+10;
    else if( (ch>='0' && ch<='9') ) return ch-'0';
    return -1;
}

int cson_cgi_urldecode_inline( char * str )
{
    unsigned char ch = 0;
    unsigned char cx1 = 0;
    unsigned char cx2 = 0;
    int decoded;
    unsigned char * pos = (unsigned char *)str;
    unsigned char * out = pos;
    unsigned char const * end;
    size_t slen = (str && *str) ? strlen(str) : 0;
    if( !slen ) return 0;
    end = pos + slen;
    for( ; pos < end; ++pos )
    {
        ch = *pos;
        if( ch == '%' )
        {
            cx1 = *(pos+1);
            /* FIXME: with only minor refactoring we can remove the
               isxdigit() calls and use cson_cgi_hexchar_to_int()
               instead, checking for a negative return value. That
               would potentially save us 2 extra function calls here.
             */
            if( isxdigit(cx1) )
            {
                cx2 = *(pos+2);
                if( isxdigit(cx2) )
                {
                    decoded = (cson_cgi_hexchar_to_int( cx1 ) * 16)
                        + cson_cgi_hexchar_to_int( cx2 );
                    *(out++) = (char)decoded;
                    pos += 2;
                    continue;
                }
                /* else fall through... */
            }
            /* else fall through... */
        }
        else if( ch == '+' )
        {
            *(out++) = ' ';
            continue;
        }
        *(out++) = ch;
    }
    *out = 0;
    return 0;
}

/**
   If PATH_INFO is set, this function splits it on '/'
   characters and creates an array out of the elements.
   The array is stored as $ENV["PATH_INFO_SPLIT"].

   Returns non-0 on error. If PATH_INFO is not set,
   0 is returned. If it is set but has no entries,
   an empty array is created.

   A return value of cson_rc.RangeError probably means that a path
   element was longer than our internal buffer size, in which case
   processing ends and PATH_INFO_SPLIT is not set. That error can
   probably be ignored by the caller, but all others are probably
   serious (e.g. AllocError).
*/
static int cson_cgi_import_path_info(cson_cgi_cx *cx)
{
    char const * pi = cson_cgi_getenv_cstr(cx, "e","PATH_INFO");
    if( NULL == pi ) return 0;
    else
    {
        cson_value * arV = cson_value_new_array();
        cson_array * ar;
        char const * head = pi;
        char const * tail = NULL;
        if( ! arV ) return cson_rc.AllocError;
        else
        {
            enum { BufSize = 128 };
            char buf[BufSize];
            cson_value * partV;
            unsigned int slen;
            int rc = 0;
            ar = cson_value_get_array(arV);
            while( cson_cgi_next_token( &head, '/', &tail ) )
            {
                slen = (tail-head);
                if( slen >= BufSize )
                {
                    rc = cson_rc.RangeError;
                    goto end_clean;
                }
                memcpy( buf, head, slen );
                buf[slen] = 0;
                cson_cgi_urldecode_inline( buf );
                partV = cson_value_new_string( buf, strlen(buf) );
                if( ! partV )
                {
                    rc = cson_rc.AllocError;
                    goto end_clean;
                }
                rc = cson_array_append( ar, partV );
                if( rc )
                {
                    cson_value_free( partV );
                    goto end_clean;
                }
                partV = NULL;
                head = tail;
                tail = NULL;
            }
            assert( 0 == rc );
            rc = cson_object_set( cx->request.env.jobj,
                                  "PATH_INFO_SPLIT",
                                  arV );
            end_clean:
            if( rc )
            {
                cson_value_free( arV );
            }
            return rc;
        }
    }
}

/**
   Imports (extern char ** environ) into cx->request.env, initializing
   cx->request.env if needed. If called multiple times the environment
   is re-read each time, but old entries which are no longer in the
   new environment are not removed from cx->request.env.

   Returns 0 on success.
*/
static int cson_cgi_import_environ(cson_cgi_cx * cx)
{
    extern char ** environ;
    int i = 0;
    char const * e = environ[0];
    char const * v = NULL;
    enum { KeyBufSize = 512 };
    char keybuf[KeyBufSize];
    char * kpos = NULL;
    int rc = 0;
    cson_value * jv = NULL;
    rc = cson_cgi_init_env_map( cx, cson_cgi_keys.ENV_SYS, &cx->request.env );
    if( 0 != rc ) return rc;
    for( ; e && *e; e = environ[++i] )
    {
        v = NULL;
        memset( keybuf, 0, KeyBufSize );
        kpos = keybuf;
        for( ; *e && ('=' != *e); ++e )
        {
            *(kpos++) = *e;
            assert( kpos < (keybuf+KeyBufSize) );
            if( kpos >= (keybuf+KeyBufSize) )
            {
                return cson_rc.RangeError;
            }
        }
        if( '=' == *e )
        {
            v = e+1;
        }
        else
        {
            v = "";
        }
        jv = cson_value_new_string( v, strlen(v) );
        if( NULL == jv )
        {
            rc = cson_rc.AllocError;
            break;
        }
        rc = cson_object_set( cx->request.env.jobj, keybuf, jv );
        if( 0 != rc ) break;
    }
    if( 0 == rc )
    {
        rc = cson_cgi_import_path_info(cx);
    }
    return rc;
}

/**
   Tries to save the current session data, if any, using the
   configured session manager.

   Returns 0 on success. If the environment has no session,
   it is treated as success but nothing is actually saved.

   If no session manager has been configured then
   cson_rc.UnsupportedError is returned.
*/
static int cson_cgi_session_save(cson_cgi_cx * cx)
{
    if( ! cx->session.mgr )
    {
        return cson_rc.UnsupportedError;
    }
    else if( !cx->session.id || !cx->session.env.jval )
    {
        return 0;
    }
    else
    {
        return cx->session.mgr->api->save( cx->session.mgr,
                                           cx->session.env.jval,
                                           cx->session.id );
    }
}

cson_cgi_cx * cson_cgi_cx_alloc()
{
    cson_cgi_cx * rc = (cson_cgi_cx *)malloc(sizeof(cson_cgi_cx));
    if( rc )
    {
        *rc = cson_cgi_cx_empty;
        rc->misc.allocStamp = rc;
    }
    return rc;
}

char cson_cgi_cx_clean( cson_cgi_cx * cx )
{
    if( !cx ) return 0;
    else
    {
        void const * allocStamp = NULL;
        if( cx->session.mgr )
        {
            cson_cgi_session_save(cx) /* ignoring error code */;
            cx->session.mgr->api->finalize( cx->session.mgr );
            cx->session.mgr = NULL;
        }
        if(NULL != cx->gc.jval)
        {
            cson_value_free( cx->gc.jval );
            cx->gc.jval = NULL;
            cx->gc.jobj = NULL;
        }
        if( cx->session.id )
        {
            free( cx->session.id );
            cx->session.id = NULL;
        }
        cson_buffer_reserve( &cx->tmpBuf, 0 );
        allocStamp = cx->misc.allocStamp;
        if( cx->opt.inStream && (stdin != cx->opt.inStream) ) fclose(cx->opt.inStream);
        if( cx->opt.outStream && (stderr != cx->opt.outStream) && (stdout != cx->opt.outStream) ) fclose(cx->opt.outStream);
        if( cx->opt.errStream && (stderr != cx->opt.errStream) && (stdout != cx->opt.errStream) ) fclose(cx->opt.errStream);
        *cx = cson_cgi_cx_empty;
        return ( allocStamp == cx )
            ? (free( cx ), 1)
            : 0;
    }
}

cson_value * cson_cgi_env_get_val( cson_cgi_cx * cx, char which, char createIfNeeded )
{
    cson_cgi_env_map * map = NULL;
    cson_value * v = NULL;
    char const * gckey = NULL;
    switch( which )
    {
      case 'c':
      case 'C':
          map = &cx->request.cookie;
          gckey = cson_cgi_keys.ENV_COOKIE;
          break;
      case 'e':
      case 'E':
          gckey = cson_cgi_keys.ENV_SYS;
          map = &cx->request.env;
          break;
      case 'g':
      case 'G':
          gckey = cson_cgi_keys.ENV_GET;
          map = &cx->request.get;
          break;
      case 'f':
      case 'F':
          gckey = cson_cgi_keys.ENV_CONFIG;
          map = &cx->config;
          break;
      case 'p':
      case 'P':
          gckey = cson_cgi_keys.ENV_POST;
          map = &cx->request.post;
          break;
      case 'a':
      case 'A':
          gckey = cson_cgi_keys.ENV_APP;
          map = &cx->clientEnv;
          break;
      case 's':
      case 'S':
          gckey = cson_cgi_keys.ENV_SESSION;
          map = &cx->session.env;
          break;
      default:
          break;
    }
    if( map )
    {
        v = map->jval;
        if( !v && createIfNeeded )
        {
            assert( NULL != gckey );
            cson_cgi_init_env_map( cx, gckey, map );
            v = map->jval;
        }
    }
    return v;
}

cson_object * cson_cgi_env_get_obj( cson_cgi_cx * cx, char which, char createIfNeeded )
{
    return cson_value_get_object( cson_cgi_env_get_val( cx, which, createIfNeeded ) );
}

/**
   Sets a variable in one of the environment objects.

   env must be the conventional character representation
   (case-insensitive) for on of the following environment objects:

   - g = GET
   - p = POST
   - e = ENV
   - c = COOKIE
   - u = USER

   On success 0 is returned and ownership of v is transfered to (or
   shared with) the appropriate environment object. On error non-zero
   is returned and ownership of v is not modified.
*/
static int cson_cgi_setenv_x( cson_cgi_cx * cx, char env, char const * key, cson_value * v )
{
    if( ! key || !*key ) return cson_rc.ArgError;
    else
    {
        cson_object * jo = cson_cgi_env_get_obj( cx, env, 1 );
        return ( NULL == jo )
            ? cson_rc.RangeError /* FIXME: expand the above code so we
                                    can distinguish between invalid
                                    env and allocation error. (Except that
                                    there is no allocation on get_obj().*/
            : cson_object_set( jo, key, v );
    }
}

int cson_cgi_setenv( cson_cgi_cx * cx, char const * key, cson_value * v )
{
    return cson_cgi_setenv_x( cx, 'a', key, v );
}

int cson_cgi_cookie_set( cson_cgi_cx * cx, char const * key, cson_value * v )
{

    if( ! key || !*key ) return cson_rc.ArgError;
    else
    {
        cson_object * jo = cson_cgi_env_get_obj( cx, 'c', 1 );
        return (NULL == jo)
            ? cson_rc.AllocError
            : cson_object_set( jo, key, v ? v : cson_value_null() );
    }
}

int cson_cgi_cookie_set2( cson_cgi_cx * cx,
                          char const * key, cson_value * v,
                          char const * domain, char const * path,
                          unsigned int expires, char secure, char httponly )
{
    if( ! key || !*key ) return cson_rc.ArgError;
    else
    {
        int rc;
        cson_value * jv = cson_value_new_object();
        cson_object * jo = cson_value_get_object(jv);
        cson_value * x = NULL;
        if( ! jo ) return cson_rc.AllocError;
        if( ! v ) v = cson_value_null() /* reminder: does not allocate */;

#define SET(KEY) if( 0 != (rc = cson_object_set( jo, KEY, x) ) ) {      \
            cson_value_free(x); \
            cson_value_free( jv ); \
            return rc; \
        }

        if( NULL != domain )
        {
            x = cson_value_new_string( domain, strlen(domain) );
            SET("domain");
        }
        if( NULL != path )
        {
            x = cson_value_new_string( path, strlen(path) );
            SET("path");
        }

        if( cson_value_is_null(v) )
        {
            x = cson_value_new_integer( 1 );
            SET("expires");
        }
        else if( expires )
        {
            x = cson_value_new_integer( (cson_int_t) expires );
            SET("expires");
        }
        if( secure )
        {
            x = cson_value_new_bool(secure);
            SET("secure");
        }
        if( httponly )
        {
            x = cson_value_new_bool(httponly);
            SET("httponly");
        }
#undef SET
        rc = cson_cgi_cookie_set( cx, key, jv );
        if( 0 != rc )
        {
            cson_value_free( jv );
        }
        else
        { /* set "value" last so that we can avoid tricky
             ownership/lifetime problems in error cases.
          */
            if( 0 != (rc = cson_object_set( jo, "value", v) ) )
            { /* remove the cookie. Note that this particular case
                 does not remove it from the HTTP client. In order to do that
                 we have to keep the existing path/domain/etc info.
              */
                cson_object * cookies = cson_cgi_env_get_obj( cx, 'c', 0 );
                if( cookies )
                {
                    cson_object_set( cookies, key, cson_value_null() )
                        /* Ignore error code, since we have no fallback
                           and cson_value_null() does not allocate.
                           Worst-case is that removing it fails, but when we
                           emit the cookie headers that cookie will be skipped
                           because it has no "value" field.
                        */
                        ;
                }
            }
        }
        return rc;
    }
}

cson_value * cson_cgi_getenv( cson_cgi_cx * cx, char const * fromWhere, char const * key )
{
    cson_value * jv = NULL;
    cson_object * map = NULL;
    if( (NULL == fromWhere) || !*fromWhere ) fromWhere = CSON_CGI_GETENV_DEFAULT;
    if( !key || !*key ) return NULL;
    for( ; *fromWhere ; ++fromWhere )
    {
        map = cson_cgi_env_get_obj( cx, *fromWhere, 0 );
        if( (NULL == map) && (('r'==*fromWhere)||('R'==*fromWhere)) )
        {
            jv = cson_cgi_getenv( cx, "gpc", key );
        }
        if( NULL != jv ) /* only in 'R' case */ break;
        else if( NULL == map ) continue /* invalid character or NULL map */;
        jv = cson_object_get( map, key );
        if( NULL != jv ) break;
    }
    return jv;
}


int cson_cgi_response_header_add( cson_cgi_cx * cx, char const * key, cson_value * v )
{
    int rc = 0;
    if( !cx || ! key || !*key ) return cson_rc.ArgError;
    rc = cson_cgi_init_env_map( cx, cson_cgi_keys.RESPONSE_HEADERS, &cx->response.headers );
    if( 0 == rc )
    {
        assert( NULL != cx->response.headers.jobj );
        rc = cson_object_set( cx->response.headers.jobj, key, v );
    }
    return rc;
}


char cson_cgi_is_jsonp(cson_cgi_cx * cx)
{
    if( ! cx ) return 0;
    else if( cx->misc.isJSONP < 0 )
    { /* guess */
        cx->misc.isJSONP = (NULL == cson_cgi_getenv( cx, "agp", CSON_CGI_KEY_JSONP ))
            ? 0 : 1;
    }
    return cx->misc.isJSONP;
}

void cson_cgi_enable_jsonp( cson_cgi_cx * cx, char b )
{
    if( cx ) cx->misc.isJSONP = b ? 1 : 0;
}

char const * cson_cgi_guess_content_type(cson_cgi_cx * cx)
{
    char const * cset;
    char doUtf8;
    cset = getenv("HTTP_ACCEPT_CHARSET");
    doUtf8 = ((NULL == cset) || (NULL!=strstr("utf-8",cset)))
        ? 1 : 0;
    if( cson_cgi_is_jsonp(cx) )
    {
        return doUtf8
            ? "application/javascript; charset=utf-8"
            : "application/javascript";
    }
    else
    {
        /*
          Content-type

          If the browser does not sent an ACCEPT for application/json
          then we fall back to text/plain.
        */
        char const * cstr;
        cstr = getenv("HTTP_ACCEPT");
        if( NULL == cstr )
        {
            return doUtf8
                ? "application/json; charset=utf-8"
                : "application/json";
        }
        else
        {
            if( strstr( cstr, "application/json" )
                || strstr( cstr, "*/*" ) )
            {
                return doUtf8
                    ? "application/json; charset=utf-8"
                    : "application/json";
            }
            else
            {
                return "text/plain";
            }
        }
    }
}


/**
   URL-encodes src to dest and NUL-terminates it. dest must be at
   least *destLen bytes long. Upon a successful return, *destLen
   will be modified to hold the new string's length.

   Returns 0 on success. On error dest might be partially populated.

   Returns cson_rc.RangeError if dest is not long enough to hold
   the conversion and a terminating NUL.
*/
static int cson_cgi_urlencode( char const * src, char * dest_, size_t * destLen )
{
#define needs_escape \
    ( (ch >= 32 && ch <=47)                       \
      || ( ch>=58 && ch<=64)                      \
      || ( ch>=91 && ch<=96)                      \
      || ( ch>=123 && ch<=126)                    \
      || ( ch<32 || ch>=127)                      \
      )
    char const * pos = src;
    char ch;
    size_t dpos = 0;
    char * dest = dest_;
    static char const * hex = "0123456789ABCDEF";
    if( ! dest || !destLen ) return cson_rc.RangeError;
    for( ; pos && *pos; ++pos )
    {
        ch = *pos;
        if( ! needs_escape )
        {
            if( ++dpos >= *destLen ) return cson_rc.RangeError;
            *(dest++) = ch;
            continue;
        }
        else
        {
            if( (dpos+=3) >= *destLen ) return cson_rc.RangeError;
            *(dest++) = '%';
            *(dest++) = hex[((ch>>4)&0xf)];
            *(dest++) = hex[(ch&0xf)];
        }
    }
    if( ++dpos >= *destLen ) return cson_rc.RangeError;
    *dest = 0;
    *destLen = dest - dest_;
    return 0;
#undef needs_escape
}

/**
   If orig is one of the types (string,double,bool,undef,null) then
   a pointer to its string representation is returned, else NULL
   is returned.

   For non-string types, dest must be at least destLen bytes of memory, and
   if destLen is not long enough to hold the string form then NULL is returned.

   On success a pointer to a string is returned. It will be one of:

   - if orig is-a string then it's underlying string.

   - for (double,integer,bool,undef,null), dest will be returned. The encoded
   form is decimal for (double,integer), the number 0 or 1 for bool, and the
   number 0 for (undef,null).

   Ownership of dest is not modified by this call.

   The returned value is valid until either orig or dest are modified.

   On error dest is not modified. Dest is also not modified if orig
   is-a string, as its own string bytes are returned instead.
*/
static char const * cson_cgi_pod_to_string( cson_value const * orig,
                                            char * dest, unsigned int destLen )
{
    if( ! orig || !dest || !destLen ) return NULL;
    else
    {/* FIXME? use cson's output support for the numeric types. i
        _think_ those bits might not be in the public API, though.
        We could use it for serializing objects/arrays, in any case.
      */
        enum { NumBufSize = 80 };
        if( cson_value_is_string(orig) )
        {
            cson_string const * jstr = cson_value_get_string(orig);
            assert( NULL != jstr );
            return cson_string_cstr( jstr ); 
        }
        else if( cson_value_is_integer(orig) )
        {
            char tmp[NumBufSize] = {0};
            int const sc = sprintf( tmp, "%"CSON_INT_T_PFMT, cson_value_get_integer(orig));
            if( sc <= 0 ) return NULL;
            else if( (unsigned int)sc >= destLen ) return NULL;
            else
            {
                strcpy( dest, tmp );
                return dest;
            }
        }
        else if( cson_value_is_double(orig) )
        {
            char tmp[NumBufSize] = {0};
            int const sc = sprintf( tmp, "%"CSON_DOUBLE_T_PFMT, cson_value_get_double(orig));
            if( sc <= 0 ) return NULL;
            else if( (unsigned int)sc >= destLen ) return NULL;
            else
            {
                strcpy( dest, tmp );
                if(1)
                { /* Strip trailing zeroes... */
                    unsigned int urc = strlen(dest);
                    char * pos = dest + urc - 1;
                    for( ; ('0' == *pos) && urc && (*(pos-1) != '.'); --pos, --urc )
                    {
                        *pos = 0;
                    }
                    assert(urc && *pos);
                }
                return dest;
            }
        }
        else if( cson_value_is_bool( orig ) )
        {
            char const bv = cson_value_get_bool(orig);
            if( destLen < 2 ) return NULL;
            *dest = bv ? '1' : '0';
            *(dest+1) = 0;
            return dest;
        }
        else if( cson_value_is_null( orig ) || cson_value_is_undef( orig ) )
        {
            if( destLen < 2 ) return NULL;
            *dest = '0';
            *(dest+1) = 0;
            return dest;
        }
        else
        {
            return NULL;
        }
    }
}


/**
   Writes an RFC822 timestamp string to dest, which must be at least destLen bytes long.
   On success returns dest, else NULL. destLen must be at least 31.
*/
static char * cson_cgi_rfc822_timedate( time_t now, char * dest, unsigned int destLen )
{
    static const char * dayNames[] = 
        {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat",
         0 };
    static const char * monthNames[] =
        {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
         "Jul", "Aug", "Sep", "Oct", "Nov", "Dec",
         0};

    struct tm * t = (dest && (destLen>30)) ? gmtime(&now) : NULL;
    if( ! t || (destLen<31) ) return NULL;
    else
    {
        int const rc = sprintf( dest,
                                "%s, %d %s %02d %02d:%02d:%02d GMT",
                                dayNames[t->tm_wday], t->tm_mday,
                                monthNames[t->tm_mon],
                                t->tm_year+1900, t->tm_hour,
                                t->tm_min, t->tm_sec
                                );
        assert( (rc>0) && ((unsigned int)rc) < destLen );
        return dest;
    }
}

/**
   Outputs the cookie-specific HTTP headers.

   Returns 0 on success.
*/
static int cson_cgi_response_output_cookies(cson_cgi_cx * cx)
{
    cson_kvp * kvp = NULL;
    cson_object * jo = NULL;
    cson_object_iterator iter = cson_object_iterator_empty;
    assert(cx);
    jo = cx->request.cookie.jobj;
    if( ! jo ) return 0;
    else
    {
        enum { CookieBufSize = 1024 * 8,
               ValBufSize = 1024 * 4,
               TSBufSize = 32
        };
        char cookieBuf[CookieBufSize] = {0} /* buffer for whole cookie string */;
        char valBuf[ValBufSize] = {0} /* buffer for value encoding */;
        char urlBuf[ValBufSize] = {0} /* buffer for urlencoding */;
        char tsBuf[TSBufSize] = {0} /* buffer for expiry timestamp */;
        int rc = cson_object_iter_init( jo, &iter );
        assert( CookieBufSize > ValBufSize );
        if( 0 != rc ) return rc;
        while( (kvp = cson_object_iter_next(&iter)) )
        {
            cson_string const * key = cson_kvp_key(kvp);
            cson_value const * val = cson_kvp_value(kvp);
            if( cson_value_is_null(val) )
            {
#if 1
                cson_cgi_printf(cx,"Set-Cookie: %s=; Expires=Thu, 01-Jan-1970 00:00:01 GMT\r\n", cson_string_cstr(key));
#else
                cson_cgi_printf(cx,"Set-Cookie: %s=\r\n", cson_string_cstr(key));
#endif
                continue;
            }
            if( cson_value_is_object(val) )
            {
                /*
                  Accept in Object in the form:

                  {
                  value: VALUE,
                  domain: string,
                  path: string,
                  secure: bool,
                  httponly: bool,
                  expires: integer
                  }
                 */
                cson_object const * obj = cson_value_get_object( val );
                cson_value const * cv = cson_object_get( obj, "value" );
                char const * valstr = NULL;
                char const isNull = !cv || cson_value_is_null( cv );
                if( isNull )
                {
                    cson_cgi_printf(cx, "Set-Cookie: %s=", cson_string_cstr(key));
                }
                else
                {
                    /* FIXME: streamify urlencode so we can get around fixed buffer size. */
                    valstr = cson_cgi_pod_to_string( cv, valBuf, ValBufSize );
                    if( ! valstr ) continue;
                    else
                    {
                        size_t bSize = ValBufSize;
                        memset( urlBuf, 0, ValBufSize );
                        if( 0 != cson_cgi_urlencode( valstr, urlBuf, &bSize ) )
                        {
                            /* buffer is too small. Skip it. */
                            continue;
                        }
                        assert( bSize <= ValBufSize );
                        cson_cgi_printf(cx, "Set-Cookie: %s=%s", cson_string_cstr(key), urlBuf);
                    }
                }

#define DOPART(KEY,KEY2) cv = cson_object_get( obj, KEY );  \
                if( cv ) { \
                    valstr = cson_cgi_pod_to_string( cv, valBuf, ValBufSize ); \
                    if( valstr ) { \
                        cson_cgi_printf( cx, "; "KEY2"=%s", valstr );  \
                    } } (void)0
                DOPART("domain","Domain");
                DOPART("path","Path");
#undef DOPART

                cv = cson_object_get( obj, "expires" );
                if( cv || isNull )
                {
                    cson_int_t const intVal = isNull ? 1 : cson_value_get_integer(cv);
                    if( intVal )
                    {
                        valstr = cson_cgi_rfc822_timedate( (time_t)intVal, tsBuf, TSBufSize );
                        if( valstr )
                        {
                            cson_cgi_printf( cx, "; Expires=%s", valstr );
                        }
                    }
#if 0
                    else if( cson_value_is_string(cv) )
                    {
                        /* TODO?: assume it's already propery formatted. */
                    }
                    else
                    {
                        /* skip it.*/
                    }
#endif
                }
                cv = cson_object_get( obj, "secure" );
                if( cson_value_get_bool(cv) )
                {
                    cson_cgi_printf( cx, "; Secure" );
                }
                
                cv = cson_object_get( obj, "httponly" );
                if( cson_value_get_bool(cv) )
                {
                    cson_cgi_printf( cx, "; HttpOnly" );
                }
                cson_cgi_puts(cx, "\r");
            }
            else
            {
                char const * valstr;
                memset( valBuf, 0, ValBufSize );
                valstr = cson_cgi_pod_to_string( val, valBuf, ValBufSize );
                if( ! valstr ) continue;
                else
                {
                    size_t bSize = CookieBufSize;
                    memset( cookieBuf, 0, CookieBufSize );
                    rc = cson_cgi_urlencode( valstr, cookieBuf, &bSize );
                    if( 0 != rc )
                    {
                        /* too beaucoup. skip it */
                        continue;
                    }
                    assert( bSize < CookieBufSize );
                    cson_cgi_printf(cx,"Set-Cookie: %s=%s\r\n", cson_string_cstr(key), cookieBuf);
                }
            }
        }
        return 0;
    }

}
int cson_cgi_response_output_headers(cson_cgi_cx * cx)
{
    enum { BufSize = 64 };
    cson_object * jo = NULL;
    int rc;
    rc = cson_cgi_printf(cx, "Content-type: %s\r\n", cson_cgi_guess_content_type(cx) );
    if( rc <= 0 ) return rc;
    rc = cson_cgi_puts(cx, "Status: 200 OK\r");
    if( rc <= 0 ) return rc;
    jo = cx->response.headers.jobj;
    if( jo )
    {
        char buf[BufSize] = {0};
        cson_object_iterator iter = cson_object_iterator_empty;
        cson_kvp * kvp;
        cson_string const * key;
        cson_value const * val;
        char const * valcstr;
        rc = cson_object_iter_init( jo, &iter );
        if( 0 != rc ) return rc;
        while( (kvp = cson_object_iter_next(&iter)) )
        {
            key = cson_kvp_key(kvp);
            val = cson_kvp_value(kvp);
            valcstr = cson_cgi_pod_to_string( val, buf, BufSize );
            if( ! valcstr ) continue;
            assert( NULL != key );
            assert( NULL != val );
            cson_cgi_printf(cx, "%s: %s\r\n",
                            cson_string_cstr(key),
                            valcstr ? valcstr : "");
        }
    }
    rc = cson_cgi_response_output_cookies(cx);
    return rc;
}

int cson_cgi_response_output_root(cson_cgi_cx * cx)
{
    return ( !cx || !cx->response.root )
        ? cson_rc.ArgError
        : cson_output_FILE( cx->response.root, cx->opt.outStream, &cx->opt.outOpt );
}

int cson_cgi_response_output_all(cson_cgi_cx * cx)
{
    int rc = 0;
    char isJP = 0;
    char doHeaders = cx->opt.httpHeadersMode;
    if( NULL == cx->response.root )
    {
        return cson_rc.ArgError;
    }
    isJP = cson_cgi_is_jsonp(cx);
    if( doHeaders < 0 )
    {
        if( NULL!=getenv("GATEWAY_INTERFACE") )
        {
            doHeaders = 1;
        }
    }
    if( doHeaders > 0 )
    {
        rc = cson_cgi_response_output_headers(cx);
        if( 0 == rc )
        {
            cson_cgi_puts(cx,"\r")/*yes, putS, not putCHAR!*/;
        }
        else return rc;
    }
    if( isJP )
    {
        cson_cgi_printf(cx,"%s(", "FIXME_JSONP_CALLBACK_NAME" );
    }
    rc = cson_cgi_response_output_root(cx);
    if( 0 == rc )
    {
        if( isJP )
        {
            cson_cgi_putchar(cx,')');
        }
        cson_cgi_putchar(cx,'\n');
        fflush( cx->opt.outStream );
    }
    return rc;
}

/**
   Parses inp as a delimited list, separated by the given
   separator character. Each item in the list is treated
   as a key/value pair in the form KEY=VALUE, and inserted
   into the target cson_object (which must not be NULL).

   This is intended for parsing HTTP GET-style parameter lists.

   If doUrlDecode is true (non-zero) then the VALUE part of the
   key/value pair gets url-decoded before insertion. (FIXME? Also
   decode the keys?)

   If firstOneWins is non-0 then if a given key in the parameters is
   duplicated, entries after the first are ignored. If it is 0 then
   the "last one wins." This is basically a workaround for when we
   have multiple session ID cookies hanging around :/.
   
   On success it returns 0.

   If a given key contains the string "[]", that part is stripped and
   the entry is treated like an array element. e.g. a query string of
   "a[]=3&a[]=7" would result in an array property named "a" with the
   (string) entries ("3", "7").
   
*/
static int cson_cgi_parse_param_list( cson_cgi_cx * cx,
                                      cson_object * tgt,
                                      char const * inp,
                                      char separator,
                                      char doUrlDecode,
                                      char firstOneWins)
{
    if( ! tgt || !separator ) return cson_rc.ArgError;
    else if( !inp || !*inp ) return 0;
    else
    {
        char const * head = inp;
        char const * tail = NULL;
        char * out = NULL;
        unsigned int inLen = strlen( inp );
        unsigned int valLen;
        cson_value * jval = NULL;
        cson_value * listV = NULL;
        cson_array * list = NULL;
        int rc = cson_buffer_reserve( &cx->tmpBuf, inLen+1 );
        if( 0 != rc ) return rc;
        while( cson_cgi_next_token( &head, separator, &tail ) )
        {
            char const * key = head;
            char * value = NULL;
            rc = 0;
            if( head == tail ) break;
            out = (char *)cx->tmpBuf.mem;
            memset( cx->tmpBuf.mem, 0, cx->tmpBuf.capacity );
            for( ; (key<tail) && *key && isspace(*key); ++key )
            {
                /* strip leading spaces in the key name
                   (happens in cookie values). */
            }
            if( key==tail ) break;
            else if( '='==*key )
            {
                /* all-space key. Just skip it. */
                goto next_iter;
            }
            /* Write the key part to the buffer... */
            for( ; (key<tail) && *key && ('='!=*key); ++key ) {
                *(out++) = *key;
            }
            *(out++) = 0;
            if( '=' == *key )
            {
                ++key;
            }
            value = out;
            valLen = 0;
            /* Write the value part to the buffer... */
            for( ; (key<tail) && *key; ++key, ++valLen ) {
                *(out++) = *key;
            }
            key = (char const *)cx->tmpBuf.mem;
            if( firstOneWins && (NULL != cson_object_get( tgt, key )) )
            {
                goto next_iter;
            }
            if( doUrlDecode && valLen )
            {
                cson_cgi_urldecode_inline( value );
            }
            /*MARKER("key=[%s], valLen=%u, value=[%s]\n", key, valLen, value );*/
            jval = cson_value_new_string( value, valLen );
            if( NULL == jval )
            {
                rc = cson_rc.AllocError;
                goto the_end;
            }
            if( NULL != (out = strstr(key,"[]")) )
            { /* Treat key as an array entry, like PHP does... */
                cson_value * freeThisOnErr = NULL;
                *out = 0;
                list = NULL;
                listV = cson_object_get( tgt, key );
                if( listV )
                {
                    if( ! cson_value_is_array( listV ) )
                    {
                        /* skip it to avoid hosing a different entry. */
                        cson_value_free( jval );
                        jval = NULL;
                        goto next_iter;
                    }
                }
                else
                { /* create a new array to hold the value */
                    listV = cson_value_new_array();
                    if( ! listV )
                    {
                        cson_value_free( jval );
                        rc = cson_rc.AllocError;
                        goto the_end;
                    }
                    rc = cson_object_set( tgt, key, listV );
                    if( 0 != rc )
                    {
                        cson_value_free( listV );
                        cson_value_free( jval );
                        goto the_end;
                    }
                    freeThisOnErr = listV;
                }
                list = cson_value_get_array( listV );
                assert( NULL != list );
                rc = cson_array_append( list, jval );
                if( 0 != rc )
                {
                    cson_value_free( jval );
                    cson_value_free( freeThisOnErr );
                    goto the_end;
                }
            }
            else
            {
                rc = cson_object_set( tgt, key, jval );
                if( 0 != rc )
                {
                    cson_value_free( jval );
                    goto the_end;
                }
            }
            next_iter:
            head = tail;
            tail = NULL;
        }
        the_end:
        cson_buffer_reserve( &cx->tmpBuf, 0 );
        return rc;
    }
}
                             

/**
   Parses key/value pairs from a QUERY_STRING-formatted
   string.

   Returns 0 on success. The "most likely" error condition, in terms
   of potential code paths, is is an allocation error.
   
   TODO: if the key part of any entry ends with "[]", treat it as an
   array entry, like PHP does.
*/
static int cson_cgi_parse_query_string( cson_cgi_cx * cx, char const * qstr )
{
    cson_object * env = NULL;
    if( !qstr || !*qstr ) return 0;
    assert(cx);
    env = cson_cgi_env_get_obj( cx, 'g', 1 );
    if( NULL == env ) return cson_rc.AllocError /* guess! */;
    return cson_cgi_parse_param_list( cx, env, qstr, '&', 1, 0 );
}

#if CSON_CGI_ENABLE_POST_FORM_URLENCODED
static int cson_cgi_parse_post_urlencoded( cson_cgi_cx * cx, char const * qstr )
{
    cson_object * env = NULL;
    if( !qstr || !*qstr ) return 0;
    assert(cx);
    env = cson_cgi_env_get_obj( cx, 'p', 1 );
    if( NULL == env ) return cson_rc.AllocError /* guess! */;
    return cson_cgi_parse_param_list( cx, env, qstr, '&', 1, 0 );
}
#endif

/**
   Like cson_cgi_parse_query_string(), but expects qstr to be in COOKIE
   format.
*/
static int cson_cgi_parse_cookies( cson_cgi_cx * cx, char const * qstr )
{
    cson_object * env = NULL;
    if( !qstr || !*qstr ) return 0;
    assert(cx);
    env = cson_cgi_env_get_obj(cx, 'c', 1 );
    if( NULL == env ) return cson_rc.AllocError /* guess! */;
    return cson_cgi_parse_param_list( cx, env, qstr, ';', 1, 1 );
}


/**
   Initializes cx->argv.jval and cx->argv.jarr, adds them to the
   garbage collector, then copies argv to cx->argv.jarr as an
   array of JSON strings.

   Returns 0 on success.

   Results are undefined if argv is not a properly initialized array
   of NUL-terminated strings with at least argc entries.

   If argc is 0 or less then cx->argv is still initialized but has
   a length of 0.

   After the first call, further arguments are appended to the current
   list.
*/
static int cson_cgi_init_argv( cson_cgi_cx * cx, int argc, char const * const * argv )
{
    int rc = 0;
    int i;
    assert( NULL != cx->gc.jobj );
    if( cx->argv.jval == NULL )
    {
        cson_value * v = cson_value_new_array();
        if( NULL == v ) return cson_rc.AllocError;
        rc = cson_cgi_gc_add( cx, cson_cgi_keys.ENV_ARGV, v, 1 );
        if( 0 != rc )
        {
            /* reminder: v was freed by cson_cgi_gc_add() */
            return rc;
        }
        cx->argv.jval = v;
        cx->argv.jarr = cson_value_get_array( v );
        assert( NULL != cx->argv.jarr );
    }
    for( i = 0; i < argc; ++i )
    {
        char const * arg = argv[i];
        cson_value * vstr = cson_value_new_string( arg ? arg : "",
                                                   arg ? strlen(arg) : 0 );
        if( NULL == vstr ) return cson_rc.AllocError;
        rc = cson_array_append( cx->argv.jarr, vstr );
        if( 0 != rc )
        {
            cson_value_free( vstr );
            break;
        }
    }
    return rc;
}

typedef struct CgiPostReadState_ {
    FILE * fh;
    unsigned int len;
    unsigned int pos;
} CgiPostReadState;

static int cson_data_source_FILE_n( void * state, void * dest, unsigned int * n )
{
    if( ! state || !dest || !n ) return cson_rc.ArgError;
    else
    {
        CgiPostReadState * st = (CgiPostReadState *)state;
        if( st->pos >= st->len )
        {
            *n = 0;
            return 0;
        }
        else if( !*n || ((st->pos + *n) > st->len) ) return cson_rc.RangeError;
        else
        {
            unsigned int rsz = (unsigned int)fread( dest, 1, *n, st->fh );
            if( ! rsz )
            {
                *n = rsz;
                return feof(st->fh) ? 0 : cson_rc.IOError;
            }
            else
            {
                *n = rsz;
                st->pos += *n;
                return 0;
            }
        }
    }
}


static int cson_cgi_parse_POST_JSON(cson_cgi_cx * cx, FILE * src, unsigned int contentLen)
{
    cson_value * jv = NULL;
    int rc = 0;
    CgiPostReadState state;
    cson_parse_info pinfo = cson_parse_info_empty;
    assert( 0 != contentLen );
    assert( NULL == cx->request.post.jval );
    state.fh = src;
    state.len = contentLen;
    state.pos = 0;
    rc = cson_parse( &jv, cson_data_source_FILE_n, &state, NULL, &pinfo );
    if( rc )
    {
#if 0
        fprintf(stderr, "%s: Parsing POST as JSON failed: code=%d (%s) line=%u, col=%u\n",
                __FILE__, rc, cson_rc_string(rc), pinfo.line, pinfo.col );
#endif
        return rc;
    }
    rc = cson_cgi_gc_add( cx, cson_cgi_keys.ENV_POST, jv, 1 );
    if( 0 == rc )
    {
        cx->request.post.jval = jv;
        cx->request.post.jobj = cson_value_get_object( jv );
        assert( cx->request.post.jobj && "FIXME: also support an Array as POST data node." ); 
    }
    return rc;
}

static int cson_cgi_init_POST(cson_cgi_cx * cx)
{
    if( ! cx || !cx->opt.inStream ) return cson_rc.ArgError;
    else
    {
        FILE * src = cx->opt.inStream;
        char const * ctype = cson_string_cstr( cson_value_get_string( cson_cgi_getenv( cx, "e", "CONTENT_TYPE" ) ) );
        if( NULL == ctype ) return 0;
        else
        {
            char const * clen = cson_string_cstr( cson_value_get_string( cson_cgi_getenv( cx, "e", "CONTENT_LENGTH" ) ) );
            if( NULL == clen ) return cson_rc.ArgError;
            else
            {
                char * endpt = NULL;
                long len = strtol( clen, &endpt, 10 );
                if( (endpt && *endpt) || (len<=0) ) return cson_rc.RangeError;
#if CSON_CGI_ENABLE_POST_FORM_URLENCODED
                else if( 0 == strncmp(ctype,"application/x-www-form-urlencoded",33) )
                {
                    cson_buffer buf = cson_buffer_empty;
                    int rc = cson_buffer_fill_from( &buf, cson_data_source_FILE, src );
                    if( rc )
                    {
                        goto end_clean;
                        return rc;
                    }
                    if( buf.mem && buf.used )
                    {
#if 1
                        if( strlen((char const *)buf.mem)
                            != buf.used )
                        {
                            /* assume bad/malicious input. */
                            rc = cson_rc.RangeError;
                            goto end_clean;
                        }
#endif
                        rc = cson_cgi_parse_post_urlencoded( cx, (char const *)buf.mem );
                    }
                    end_clean:
                    cson_buffer_reserve( &buf, 0 );
                    return rc;
                }
#endif
                else if( (0 == strncmp(ctype,"application/json",16))
                         || (0 == strncmp(ctype,"text/plain",10))
                         || (0 == strncmp(ctype,"application/javascript",22))
                         )
                {
                    return cson_cgi_parse_POST_JSON(cx, src, len);
                }
                else
                {
                    return cson_rc.TypeError;
                }
            }
        }
    }
}

static int cson_cgi_init_config( cson_cgi_cx * cx, char const * fname )
{
    int rc;
    cson_value * root = NULL;
    rc = cson_parse_filename( &root, fname, NULL, NULL );
    if( 0 == rc )
    {
        assert( NULL != root );
        if( ! cson_value_is_object(root) )
        {
            cson_value_free( root );
            rc = cson_rc.TypeError;
        }
        else
        {
            rc = cson_cgi_gc_add( cx,cson_cgi_keys.ENV_CONFIG, root, 1 );
            if( 0 == rc )
            {
                cx->config.jval = root;
                cx->config.jobj = cson_value_get_object( root );
                assert( NULL != cx->config.jobj );
            }
        }
    }
    return rc;
}

static char * cson_cgi_strdup( char const * src )
{
    size_t const n = src ? strlen(src) : 0;
    char * rc = src ? (char *)malloc(n+1) : NULL;
    if( ! rc ) return NULL;
    memcpy( rc, src, n );
    rc[n] = 0;
    return rc;
}

/**
   Writes a 36-byte (plus one NUL byte) UUID value to dest. dest
   must be at least 37 bytes long. If dest is NULL this function
   has no side effects.

   Not thread-safe.
*/
void cson_cgi_generate_uuid( cson_cgi_cx * cx, char * dest )
{
    static whuuid_rng rng = {
    NULL/*rand*/,
    NULL/*cleanup*/,
    NULL/*impl*/
#if WHUUID_CONFIG_KEEP_METRICS
    ,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0}/*distribution*/
#endif
    };
    whuuid_t u = whuuid_t_empty;
    if( NULL == dest ) return;
    else if( (NULL==rng.rand) && (NULL != RNG_FILENAME) )
    { /* try to open rng file... */
        /* FIXME: we're missing a cleanup handler for the RNG_FILENAME case. */
        FILE * f = fopen(RNG_FILENAME, "rb");
        if( NULL != f )
        {
            rng = whuuid_rng_FILE;
            rng.impl = f;
        }
    }
    if( NULL == rng.rand )
    { /* fall back to LC rng */
        extern char ** environ;
        void * addr;
        unsigned long seed;
        rng = whuuid_rng_lcrng;
        addr = malloc(
                      (((unsigned long)environ) % 13) + 9
                      );
        free(addr) /* but keep the address as a seed value */;
        seed = (unsigned long)addr * (unsigned long)time(NULL);
        rng.impl = (void *)seed;
    }
    whuuid_fill_rand( &u, &rng );
    whuuid_to_string( &u, dest );
}

char const * cson_cgi_session_id(cson_cgi_cx * cx)
{
    return cx ? cx->session.id : NULL;
}


static int cson_cgi_init_session_mgr(cson_cgi_cx * cx)
{
    /*
       Check for this config structure:

       {
       manager:"mgrID",
       managers:{
           mgrID:{
               sessionDriver: "back-end-name" (e.g. "cpdo" or "file"),
               ... back-end-specific options ...
           },
           otherManager: { ... }
       }
    */
    cson_object const * conf = cson_cgi_env_get_obj(cx,  'f', 0 );
    cson_string const * aString;
    cson_value const * optV = NULL;
    cson_object const * optObj = NULL;
    if( NULL == conf ) return 0;
    assert( cx && !cx->session.mgr );

    /* get "manager" part... */
    aString = cson_value_get_string( cson_object_get_sub( conf, "session.manager", '.' ) );
    if( NULL == aString ) return 0;

    /* Fetch that manager config ... */
    optV = cson_object_get_sub( conf, "session.managers", '.' );
    if( optV )
    {
        optV = cson_object_get( cson_value_get_object( optV ), cson_string_cstr( aString ) );
    }
    optObj = cson_value_get_object( optV );
    if( ! optObj ) return 0;

    /* Get the "sessionDriver" part ... */
    aString = cson_value_get_string( cson_object_get( optObj, "sessionDriver" ) );
    if( NULL == aString ) return 0;

    return cson_sessmgr_load( cson_string_cstr(aString), &cx->session.mgr, optObj );
}


static char const * cson_cgi_get_session_key(cson_cgi_cx * cx)
{
    cson_object const * conf = cson_cgi_env_get_obj( cx, 'f', 0 );
    char const * sessKey = CSON_CGI_KEY_SESSION;
    assert( NULL != cx );
    if( conf )
    {
        cson_string const * k = cson_value_get_string( cson_object_get_sub( conf, "session.cookieName", '.' ) );
        char const * ck = k ? cson_string_cstr(k) : NULL;
        if( ck ) sessKey = ck;
    }
    return sessKey;
}

static int cson_cgi_gen_session_id(cson_cgi_cx * cx)
{
    char buf[37] = {0};
    if( cx->session.id )
    {
        free( cx->session.id );
        cx->session.id = NULL;
    }
    cson_cgi_generate_uuid( cx, buf );
    cx->session.id = cson_cgi_strdup( buf );
    return ( NULL == cx->session.id )
        ? cson_rc.AllocError
        : 0;
}

static int cson_cgi_init_session( cson_cgi_cx * cx, char const * forceID )
{
    char const * idstr;
    char const * sessKey;
    int rc = cson_cgi_init_session_mgr(cx);
    if( 0 != rc ) return rc;
    else if( NULL == cx->session.mgr ) return 0
        /* treat as non-fatal error */;
    sessKey = cson_cgi_get_session_key(cx);
    assert( sessKey && *sessKey );
    /* Try to get the session ID ... */
    idstr = (forceID && *forceID)
        ? forceID
        : cson_string_cstr( cson_value_get_string( cson_cgi_getenv( cx, "cegp", sessKey ) ) );
    if( NULL == idstr )
    { /* Generate a session ID but defer creation of the session
         object until the client does it. If they never use it,
         we won't bother saving the session.
      */
        rc = cson_cgi_gen_session_id(cx);
        if( 0 != rc ) return rc;
    }
    else
    { /* try to load the session */
        cson_value * sessV = NULL;
        free( cx->session.id );
        cx->session.id = cson_cgi_strdup( idstr );
        if( ! cx->session.id ) return cson_rc.AllocError;
        rc = cx->session.mgr->api->load( cx->session.mgr, &sessV,
                                         cx->session.id );
        if( (0 == rc) && sessV )
        {
            rc = cson_cgi_gc_add( cx, cson_cgi_keys.ENV_SESSION, sessV, 1 );
            if( 0 != rc )
            { /* almost certainly an alloc error */
                return rc;
            }
            cx->session.env.jval = sessV;
            cx->session.env.jobj = cson_value_get_object( sessV );
        }
        else
        {
            if( !forceID || !*forceID )
            {
                /* On load error, assume the session ID is
                   stale. Re-generate it to avoid potential future
                   collisions. This heuristic will cause us intermittent
                   grief when loading does not work for a second or three
                   due to network-related problems. Each time that
                   happens, the caller will lose his session.
                */
                rc = cson_cgi_gen_session_id(cx);
                if( 0 != rc ) return rc;
            }
        }
    }
    assert( NULL != cx->session.id );
    { /* make sure the session ID is set in the cookies and has an updated
         expiry time... */
        unsigned int expiry = 0;
        cson_object const * conf;
        cson_value * jstr = cson_value_new_string( cx->session.id,
                                                   strlen(cx->session.id) );
        if( ! jstr ) return cson_rc.AllocError;
        conf = cson_cgi_env_get_obj( cx, 'f', 0 );
        if( conf )
        {
            expiry = cson_value_get_integer( cson_object_get_sub( conf, "session.cookieLifetimeMinutes", '.' ) );
            if( expiry ) expiry *= 60 /* convert to seconds */;
        }
        if( ! expiry )
        {
            expiry = (60*60*24);
        }
        expiry += (unsigned int)time(NULL);
        
        rc = cson_cgi_cookie_set2( cx, sessKey, jstr,
                                   NULL, NULL,
                                   expiry,
                                   0/*FIXME: set 'secure' option in HTTPS mode.*/,
                                   0/*FIXME: make the httponly flag configurable*/ );
        if( 0 != rc )
        {
            cson_value_free( jstr );
            if( cson_rc.AllocError == rc ) return rc;
            rc = 0 /* else treat as non-fatal */;
        }
    }
    return rc;
}



int cson_cgi_init(cson_cgi_cx * cx, int argc, char const * const * argv, cson_cgi_init_opt * opt )
{
    int rc = 0;
    static int hasInited = 0;
    if( NULL == cx ) return cson_rc.ArgError;
    else if( NULL != cx->gc.jval )
    { /* we've already done this or object was mal-initialized... */
        return cson_rc.ArgError;
    }

    assert( NULL != CSON_CGI_GETENV_DEFAULT );

#if CSON_CGI_USE_SIGNALS
    {
        /* FIXME: use sigaction() instead of signal() */
        typedef void (*sighnd)(int);
        sighnd oldSigPipe;
        oldSigPipe = signal(SIGPIPE, SIG_IGN) /* to try avoid unclean termination if client disconnects. */;
        if( SIG_ERR == oldSigPipe )
        {
            return cson_rc.UnknownError;
        }
    }
#endif

    if( ! hasInited )
    {
        hasInited = 1;
        setlocale( LC_ALL, "C" )
            /* supposedly important for underlying JSON parser.
               FIXME: only do this init once!
            */;
    }

    cx->gc.jval = cson_value_new_object();
    if( NULL == cx->gc.jval )
    {
        return cson_rc.AllocError;
    }
    cx->gc.jobj = cson_value_get_object( cx->gc.jval );
    assert( NULL != cx->gc.jobj );

    if( opt )
    {
        cx->opt = *opt;
    }
    if( NULL == cx->opt.inStream ) cx->opt.inStream = stdin;
    if( NULL == cx->opt.outStream ) cx->opt.outStream = stdout;
    if( NULL == cx->opt.errStream ) cx->opt.errStream = stderr;

#define CHECKRC if(rc) goto end
    rc = cson_cgi_import_environ(cx);
    CHECKRC;
    rc = cson_cgi_init_argv( cx, argc, argv );
    CHECKRC;
    if( cx->opt.configObj ){
        cson_value * configV = cson_object_value( cx->opt.configObj );
        rc = cson_cgi_gc_add( cx,cson_cgi_keys.ENV_CONFIG, configV, 0 );
        if(rc) return cson_rc.AllocError;
        cx->config.jobj = cx->opt.configObj;
        cx->opt.configObj = NULL;
        cx->config.jval = configV;
    }
    else { /* read config file */
        char const * conffile = cx->opt.configFile;
        if( ! conffile )
        {
            cson_value const * v = cson_cgi_getenv( cx, "e", "CSON_CGI_CONFIG" );
            if( v && cson_value_is_string(v) )
            {
                conffile = cson_string_cstr( cson_value_get_string( v ) );
            }
        }
        if( conffile )
        {
            cson_cgi_init_config( cx, conffile )
                /* Ignore error code.

                TODO:

                - use argv[0]+".json" as the default config file.
                */
                ;
        }
    }

    rc = cson_cgi_parse_query_string( cx, getenv("QUERY_STRING") );
    CHECKRC;
    rc = cson_cgi_parse_cookies( cx, getenv("HTTP_COOKIE") );
    CHECKRC;
    rc = cson_cgi_init_POST(cx);
    if( cson_rc.AllocError == rc ) goto end
        /* this can fail for several reasons which are non-fatal. */
        ;

    if( (NULL == opt) )
    {
        /* TODO: read these values from cx->config, if available. */
        cx->opt.outOpt.indentation = 1;
        cx->opt.outOpt.addNewline = 1;
        cx->opt.outOpt.addSpaceAfterColon = 1;
        cx->opt.outOpt.indentSingleMemberValues = 1;
    }

    rc = cson_cgi_init_session( cx, opt ? opt->sessionID : NULL )
        /* ignore non-OOM error codes. Not fatal. */;
    if( cson_rc.AllocError == rc ) goto end;
    else rc = 0;

    /*
      TODOs:

      - Read form-urlencoded POST data. (Do this BEFORE
      restoring the session, so that we can get the session
      ID from there if needed.)
    */
    end:
    return rc;
#undef CHECKRC
}

int cson_cgi_replace_POST( cson_cgi_cx * cx, cson_value * newPost )
{
    if(!cx || !newPost) return cson_rc.ArgError;
    else if( !cson_value_is_object(newPost) && !cson_value_is_array(newPost)){
        return cson_rc.TypeError;
    }
    else {
        int rc = cson_cgi_gc_add( cx, cson_cgi_keys.ENV_POST, newPost, 0 )
            /* will free up previous post data */
            ;
        if(rc) return rc;
        cx->request.post.jval = newPost;
        cx->request.post.jobj = cson_value_get_object( newPost )
            /* reminder: we currently "don't do" arrays, but eventually
               should.
            */
            ;
        return 0;
    }
}

int cson_cgi_replace_GET( cson_cgi_cx * cx, cson_object * newGet )
{
    if(!cx || !newGet) return cson_rc.ArgError;
    else {
        cson_value * v = cson_object_value(newGet);
        int rc = cson_cgi_gc_add( cx, cson_cgi_keys.ENV_GET, v, 0 )
            /* will free up previous GET data */
            ;
        if(rc) return rc;
        cx->request.get.jval = v;
        cx->request.get.jobj = newGet;
        return 0;
    }
}


#undef cson_cgi_env_map_empty_m
#undef CSON_CGI_USE_SIGNALS
#undef RNG_FILENAME
