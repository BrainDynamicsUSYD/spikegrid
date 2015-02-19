#define _POSIX_C_SOURCE 200112L /* for setenv() on Linux */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <time.h>

#include "wh/cson/cson_cgi.h"
#include "wh/cson/cson_session.h"

#if 1
#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf
#else
static void noop_printf(char const * fmt, ...) {}
#define MARKER if(0) printf
#endif


char const * DUMMY_QUERY_STRING =
#if 1
    "a=b&&&c=d%20+e+%20%2a+f%20%x&list[]=1&list[]=3&list[]=7"
#else
    NULL
#endif
    ;
char const * DUMMY_COOKIES =
#if 1
    "cookie1=my%20first%20cookie; mykey=my%20value; cson_session_id=abacab"
#else
    NULL
#endif
    ;


static void dump_jval( char const * label, cson_value const * jv )
{
    static char doneit = 0;
    static cson_output_opt opt = cson_output_opt_empty_m;
    if( ! doneit )
    {
        doneit = 1;
        opt.indentation = 1;
        opt.addNewline = 1;
    }
    if( label && *label )
    {
        puts(label);
    }
    cson_output_FILE( jv, stdout, &opt );
}

static void dump_jstr( char const * label, cson_value const * jv )
{
    cson_string const * str = cson_value_get_string(jv);
    printf( "%s=[%s]\n", label, cson_string_cstr( str ) );
}

static void test_ENV(cson_cgi_cx * cx)
{
    cson_value * jv = cson_cgi_env_get_val( cx, 'e', 0 );
    cson_object * jo = cson_cgi_env_get_obj( cx, 'e', 0 );
    assert( NULL != jv );
    assert( NULL != jo );
    assert( cson_value_get_object(jv) == jo );
    dump_jval( "$ENV=", jv );

    cson_value * v = cson_object_get( jo, "HOME" );
    assert( NULL != v );
    assert( cson_cgi_getenv( cx, "e", "HOME" ) == v );
    dump_jstr( "$HOME=", v );
}

#if 0
#define MY_SESSION_ID "MYSESSION"
static void test_SESSION(cson_cgi_cx * cx)
{
    cson_object * conf = cson_cgi_env_get_obj( cx, 'f', 0 );
    assert( NULL != conf );
    cson_value * optV = cson_object_get_sub( conf, "sessionManagers/file", '/' );
    assert( NULL != optV );
    cson_sessmgr * sm = cson_sessmgr_file( cson_value_get_object( optV ) );
    assert( sm );
    cson_value * sess = cson_cgi_env_get_val( cx, 'e', 0 );
    assert( sess );
    int rc = sm->api->save( sm, sess, MY_SESSION_ID );
    assert( 0 == rc );
    MARKER("Session [%s] saved.\n", MY_SESSION_ID);
    sm->api->finalize( sm );
}
#endif

static void test_SESSION2(cson_cgi_cx * cx)
{
    cson_object * sess = cson_cgi_env_get_obj( cx, 's', 1 );
    if( sess )
    {
        cson_object_set( sess, "timestamp", cson_value_new_integer( (cson_int_t)time(NULL) ) );
        cson_object_set( sess, "hits", cson_value_new_integer(1 +
                                                              cson_value_get_integer( cson_object_get( sess, "hits" ) )
                                                              ) );
        cson_value * be = cson_object_get_sub( cson_cgi_env_get_obj( cx, 'f', 0 ),
                                                     "session.manager", '.' );
        if( be )
        {
            cson_object_set( sess, "sessionManager", be );
        }

#if 0
        cson_object_set( sess, "file", cson_value_new_string( __FILE__, strlen(__FILE__) ) );
        cson_object_set( sess, "gotMilk", cson_value_true() );
#endif
    }
}


static void test_ARGV(cson_cgi_cx * cx)
{
    cson_value * jv = cson_cgi_argv(cx);
    cson_array * ar = cson_cgi_argv_array(cx);
    assert( NULL != jv );
    assert( NULL != ar );
    assert( cson_value_get_array(jv) == ar );
    assert( cson_array_length_get( ar ) > 0 );
    dump_jval( "ARGV=", jv );
}

static void test_QUERY_STRING(cson_cgi_cx * cx)
{
    if( DUMMY_QUERY_STRING )
    {
        cson_value * v = NULL;
        dump_jval( "$GET=", cson_cgi_env_get_val( cx, 'g', 0 ) );
        v = cson_cgi_getenv( cx, "g", "a" );
        assert( NULL != v );
        dump_jstr( "$QUERY_STRING[a]=", v );
        v = cson_cgi_getenv( cx, "G", "c" );
        assert( NULL != v );
        dump_jstr( "$QUERY_STRING[c]=", v );
        v = cson_cgi_getenv( cx, NULL, "z" );
        assert( NULL == v );
    }
}

static void test_COOKIES(cson_cgi_cx * cx)
{
    if( DUMMY_COOKIES )
    {
        cson_value * v = NULL;
        v = cson_cgi_getenv( cx, "c", "cookie1" );
        assert( NULL != v );
        /* dump_jstr( "$COOKIES[cookie1]=", v ); */
        v = cson_cgi_getenv( cx, "C", "mykey" );
        assert( NULL != v );
        /* dump_jstr( "$COOKIES[mykey]=", v ); */
        v = cson_cgi_getenv( cx, NULL, "mykey" );
        assert( NULL != v );
        v = cson_cgi_getenv( cx, "g", "mykey" );
        assert( NULL == v );
        dump_jval( "$COOKIES=", cson_cgi_env_get_val( cx, 'c', 0 ) );
    }
}

void test_CGI( cson_cgi_cx * cx, char doHeaders )
{
    cson_value * jv = NULL;
    cson_object * jo = NULL;
    cson_value * jv2 = NULL;
    int rc;
    jv = cson_cgi_response_root_get( cx, 1 );
    jo = cson_value_get_object( jv );
    assert( NULL != jo );

    if( NULL != (jv2 = cson_cgi_argv(cx)) )
    {
        rc = cson_object_set( jo, "argv", jv2 );
        assert( 0 == rc );
    }

    
    cson_object * uobj = cson_cgi_env_get_obj( cx, 'a', 1 );
    assert( NULL != uobj );

    char const * contentType = cson_cgi_guess_content_type(cx);
    if( contentType )
    {
        jv2 = cson_value_new_string( contentType, strlen(contentType) );
        assert( jv2 );
    }

    if(1)
    { /* some random client data ... */
        rc = cson_object_set( uobj, "guessedOutputContentType", jv2 );
        assert( 0 == rc );
        jv2 = cson_value_new_integer( time(NULL) );
        assert( NULL != jv2 );
        rc = cson_object_set( uobj, "timestamp", jv2 );
        assert( 0 == rc );
#if 0 /* this isn't 32-bit friendly... */
        extern char ** environ;
        jv2 = cson_value_new_integer( (cson_int_t)environ );
        assert( NULL != jv2 );
        rc = cson_object_set( uobj, "pseudoRandom", jv2 );
        assert( 0 == rc );
#endif
    }

    if(1)
    {
        jv = cson_cgi_path_part(cx,0);
        if( jv )
        {
            rc = cson_object_set( uobj, "PATH_INFO_SPLIT[0]", jv );
            assert( 0 == rc );
        }

    }

    { /* copy various "environment"/namespace objects... */
        typedef struct {
            char id;
            char const * label;
        } EnvList;
        /**
           The following seemlingly duplicate (but differently ordered)
           lists are to accommodate my own personal testing methods.
        */
        EnvList elistCGI[] = {
        {'s',"$SESSION"},
        {'g',"$GET"},
        {'c',"$COOKIE"},
        {'p',"$POST"},
        {'a',"$APP"},
#if 0 /* don't leak my db password :) */
        {'f',"$CONFIG"},
#endif
        {'e',"$ENV"},
        {0,0}
        };
        EnvList elistLocal[] = {
        {'e',"$ENV"},
        {'g',"$GET"},
        {'c',"$COOKIE"},
        {'p',"$POST"},
        {'a',"$APP"},
        {'f',"$CONFIG"},
        {'s',"$SESSION"},
        {0,0}
        };
        EnvList * envs = (NULL==getenv("GATEWAY_INTERFACE"))
            ? elistLocal
            : elistCGI;
        for( ; envs->id; ++envs )
        {
            jv2 = cson_cgi_env_get_val( cx, envs->id, 0 );
            if( NULL != jv2 )
            {
                rc = cson_object_set( jo, envs->label, jv2 );
                assert( 0 == rc );
            }
        }
    }
    
    if( doHeaders )
    {
        rc = cson_cgi_response_header_add( cx, "X-MyIntHeader",
                                           cson_value_new_integer(42) );
        assert( 0 == rc );
        rc = cson_cgi_response_header_add( cx, "X-MyDoubleHeader",
                                           cson_value_new_double(42.24) );
        assert( 0 == rc );
        rc = cson_cgi_response_header_add( cx, "X-MyBoolHeader",
                                           cson_value_true() );
        assert( 0 == rc );
        rc = cson_cgi_response_header_add( cx, "X-MyNullHeader",
                                           cson_value_null() );
        assert( 0 == rc );


        if(0)
        {
            rc = cson_cgi_cookie_set2( cx, "special2", cson_value_null(),
                                       getenv("HTTP_HOST"), "/",
                                       1, 0, 1 );
            assert( 0 == rc );

        }

        if(0)
        {
            cson_int_t now = (cson_int_t)time(NULL);
            char const * sessID = "abacab";
            cson_int_t tomorrow = now + (3600 * 24);
            cson_cgi_cookie_set2( cx, "mykey", cson_value_new_integer( now ),
                                  NULL, NULL,
                                  tomorrow, 0, 0 );

            cson_cgi_cookie_set2( cx, CSON_CGI_KEY_SESSION,
                                  cson_value_new_string( sessID, strlen(sessID) ),
                                  NULL, NULL,
                                  tomorrow, 0, 0 );
        }
        
        if( 0 )
        {
            cson_int_t now = (cson_int_t)time(NULL);
            cson_int_t tomorrow = now + (3600 * 24);
            cson_object * cookies = cson_cgi_env_get_obj(cx, 'c', 0);
            cson_value * vC = cson_value_new_object();
            cson_object * props = cson_value_get_object( vC );
            assert( cookies );
            assert( NULL != props );
            cson_object_set( cookies, "specialCookie", vC );
            cson_object_set( props, "value", cson_value_new_string("hi there", 8) );
#if 1
            cson_object_set( props, "expires", cson_value_new_integer( tomorrow ) );
#else
            cson_object_set( props, "expires", cson_value_new_integer(1) );
#endif
            cson_object_set( props, "httponly", cson_value_new_bool(1));
            cson_object_set( props, "domain", cson_cgi_getenv(cx, "e","HTTP_HOST") );
#if 0
            cson_object_set( props, "path", cson_value_new_string("/", 1) );
            cson_object_set( props, "secure", cson_value_new_bool(0));
#endif
        }
    }
    rc = cson_cgi_response_output_all(cx);
    assert( 0 == rc );
}

int main( int argc, char const * const * argv )
{
    int rc;
    cson_cgi_cx CgiCx = cson_cgi_cx_empty;
    cson_cgi_init_opt iopt = cson_cgi_init_opt_empty;
    char doCGI;
    char const * str;

    doCGI = (NULL != getenv("GATEWAY_INTERFACE"));

    if( !doCGI )
    {
        iopt.outOpt.indentation = 1;
        iopt.outOpt.addSpaceAfterColon = 1;
        iopt.outOpt.indentSingleMemberValues = 1;
        iopt.sessionID = "abacab";
    }
    else
    {
        iopt.outOpt.indentation = 0;
        iopt.outOpt.addSpaceAfterColon = 0;
        iopt.outOpt.indentSingleMemberValues = 0;
    }
    iopt.httpHeadersMode = 1;
    iopt.outOpt.addNewline = 1;
    iopt.configFile = "cgi-test.json";
    
    str = getenv("QUERY_STRING");
    if(NULL == str){/*kludge for over-pedanic gcc*/}
    
    if( !doCGI )
    {
        if( DUMMY_QUERY_STRING )
        {
            setenv( "QUERY_STRING",
                    DUMMY_QUERY_STRING ? DUMMY_QUERY_STRING : "" /* kludge for gcc's pedantic warnings */,
                    1 );
        }
        
    }
    else
    {
        DUMMY_QUERY_STRING = NULL;
    }
    /* str = getenv("HTTP_COOKIE"); */
    if( !doCGI /* || !str */ )
    {
        if( DUMMY_COOKIES )
        {
            setenv( "HTTP_COOKIE",
                    DUMMY_COOKIES ? DUMMY_COOKIES : "" /* kludge for gcc's pedantic warnings */,
                    1 );
        }
    }
    else
    {
        DUMMY_COOKIES = NULL;
    }

    rc = cson_cgi_init( &CgiCx, argc, argv, &iopt );
    assert(0 == rc);
    if( doCGI )
    {
        test_SESSION2(&CgiCx);
        test_CGI( &CgiCx, 1 );
        cson_cgi_cx_clean(&CgiCx);
        return 0;
    }

    test_ENV(&CgiCx);
    test_ARGV(&CgiCx);
    if( DUMMY_COOKIES )
    {
        test_COOKIES(&CgiCx);
    }
    if( DUMMY_QUERY_STRING )
    {
        test_QUERY_STRING(&CgiCx);
    }    
    test_SESSION2(&CgiCx);
    test_CGI( &CgiCx, 1 );

    {
        MARKER("Registered session managers:\n");
        char const * const * mgr = cson_sessmgr_names();
        for( ; *mgr; ++mgr )
        {
            putchar('\t');
            puts( *mgr );
        }
    }

    cson_cgi_cx_clean(&CgiCx);
    return 0;
}
