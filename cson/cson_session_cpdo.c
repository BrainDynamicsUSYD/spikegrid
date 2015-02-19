#if CSON_ENABLE_CPDO /* we do this only in the interest of the amalgamation build */
#include "wh/cson/cson_session.h"
#include "wh/cpdo/cpdo_amalgamation.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

#if 0
#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf
#else
#define MARKER if(0) printf
#endif


static int cson_session_cpdo_load( cson_sessmgr * self, cson_value ** tgt, char const * id );
static int cson_session_cpdo_save( cson_sessmgr * self, cson_value const * root, char const * id );
static int cson_session_cpdo_remove( cson_sessmgr * self, char const * id );
static void cson_session_cpdo_finalize( cson_sessmgr * self );

static const cson_sessmgr_api cson_sessmgr_api_cpdo =
{
    cson_session_cpdo_load,
    cson_session_cpdo_save,
    cson_session_cpdo_remove,
    cson_session_cpdo_finalize
};

typedef struct cson_sessmgr_cpdo_impl cson_sessmgr_cpdo_impl;
struct cson_sessmgr_cpdo_impl
{
    cpdo_driver * drv;
    char * table;
    char * fId;
    char * fSession;
    char * fTimestamp;
};

static cson_sessmgr_cpdo_impl cson_sessmgr_cpdo_impl_empty = {
    NULL /*drv*/,
    NULL /*table*/,
    NULL /*fId*/,
    NULL /*fSession*/,
    NULL /*fTimestamp*/
};

static const cson_sessmgr cson_sessmgr_cpdo_empty =
{
    &cson_sessmgr_api_cpdo,
    NULL
};

static char * cson_session_cpdo_strdup( char const * src )
{
    size_t const n = src ? strlen(src) : 0;
    char * rc = src ? (char *)calloc(1, n+1) : NULL;
    if( ! rc ) return NULL;
    memcpy( rc, src, n );
    return rc;
}

#define IMPL_DECL \
    cson_sessmgr_cpdo_impl * impl = (self && (self->api == &cson_sessmgr_api_cpdo)) \
    ? (cson_sessmgr_cpdo_impl*)self->impl \
    : NULL;                                   \
    if( NULL == impl ) return cson_rc.ArgError


static int cson_session_cpdo_load( cson_sessmgr * self, cson_value ** root, char const * id )
{
    char const * cstr = NULL;
    cpdo_stmt * st = NULL;
    int rc = 0;
    uint32_t len = 0;
    cpdo_step_code scode;
    void const * blob = NULL;
    IMPL_DECL;
    if( ! root || !id || !*id ) return cson_rc.ArgError;
    rc = cpdo_prepare_f( impl->drv, &st,
                         "SELECT %s FROM %s WHERE %s=?",
                         impl->fSession, impl->table,
                         impl->fId, id );
    if( rc )
    {
        MARKER("Error preparing SELECT code: cpdo error #%d (%s)\n", rc, cpdo_rc_string( rc ) );
        if( cpdo_rc.CheckDbError == rc )
        {
            char const * err = NULL;
            impl->drv->api->error_info( impl->drv, &err, NULL, NULL );
            MARKER("Db says: %s\n", err );
        }
        return cson_rc.IOError;
    }
    rc = cpdo_bind_string( st, 1, id, strlen(id) );
    if( rc )
    {
        rc = cpdo_rc.UnknownError;
        goto clean;
    }
    scode = st->api->step( st );
    if( CPDO_STEP_DONE == scode )
    { /* no data */
        rc = cson_rc.NotFoundError;
        goto clean;
    }
    else if( CPDO_STEP_ERROR == scode )
    {
        rc = cson_rc.IOError;
        goto clean;
    }
    rc = st->api->get.string( st, 0, &cstr, &len );
    if( rc )
    { /* try again as a blob */
        rc = st->api->get.blob( st, 0, &blob, &len );
        if( rc )
        {
            rc = cson_rc.IOError;
            goto clean;
        }
        cstr = (char const *)blob;
    }

    if( ! cstr )
    {
        rc = cson_rc.UnknownError;
        goto clean;
    }
    else
    {
        cson_value * jv = NULL;
        rc = cson_parse_string( &jv, cstr, len, NULL, NULL );
        if( rc )
        {
            rc = cson_rc.TypeError;
            goto clean;
        }
        *root = jv;
        rc = 0;
    }
    clean:
    st->api->finalize( st );
    return rc;
   
}

static int cson_session_cpdo_save( cson_sessmgr * self, cson_value const * root, char const * id )
{
    cpdo_stmt * st = NULL;
    int rc;
    cson_buffer jbuf = cson_buffer_empty;
    IMPL_DECL;
    rc = cson_output_buffer( root, &jbuf, NULL );
    if( rc ) return rc;
#define CHECKRC if( rc ) { rc = cpdo_rc.UnknownError; goto clean; } (void)0
#if 1
    /* Reminder:

       We use the non-standard REPLACE SQL construct here. It is
       supported by MySQL and sqlite3, and those are the only drivers
       currently supported by cpdo. This saves us having to either do
       delete/insert or update-or-insert (depending on whether it's
       already there or not). Interestingly sqlite3 requires the 'INTO'
       keyword where MySQL does not.

       If using non-standard SQL makes you queasy, or cpdo actually
       does get another driver some day, use the ifdef'd-out
       implementation instead of this code.
    */
    rc = cpdo_prepare_f( impl->drv, &st,
                         "REPLACE INTO %s (%s,%s,%s) VALUES(?,?,?)",
                         impl->table, impl->fId, impl->fTimestamp, impl->fSession );
#else
    if( ! root || !id || !*id ) return cson_rc.ArgError;
    rc = cson_session_cpdo_remove( self, id );
    if( rc ) return rc;
    rc = cpdo_prepare_f( impl->drv, &st,
                         "INSERT INTO %s (%s,%s,%s) VALUES(?,?,?)",
                         impl->table, impl->fId, impl->fTimestamp, impl->fSession );
#endif
    CHECKRC;
    rc = cpdo_bind_string( st, 1, id, strlen(id) );
    CHECKRC;
    rc = cpdo_bind_int32( st, 2, (int32_t)time(NULL) );
    CHECKRC;
    rc = cpdo_bind_string( st, 3, (char const *)jbuf.mem, jbuf.used );
    if( rc )
    { /* try again as blob */
        rc = cpdo_bind_blob( st, 3, jbuf.mem, jbuf.used );
    }
    CHECKRC;
#undef CHECKRC
    if( CPDO_STEP_ERROR == st->api->step(st) )
    {
        rc = cson_rc.IOError;
        goto clean;
    }
    clean:
    if( st ) st->api->finalize( st );
    cson_buffer_reserve( &jbuf, 0 );
    return rc;
}

static int cson_session_cpdo_remove( cson_sessmgr * self, char const * id )
{
    cpdo_stmt * st = NULL;
    int rc;
    IMPL_DECL;
    if( !id || !*id ) return cson_rc.ArgError;
    rc = cpdo_prepare_f( impl->drv, &st,
                         "DELETE FROM %s WHERE %s=?",
                         impl->table, impl->fId );
    if( rc )
    {
        rc = cson_rc.IOError;
        goto clean;
    }
    rc = cpdo_bind_string( st, 1, id, strlen(id) );
    if( rc ) { rc = cpdo_rc.UnknownError; goto clean; }
    if( CPDO_STEP_ERROR == st->api->step( st ) )
    {
        rc = cson_rc.IOError;
        goto clean;
    }

    clean:
    if( st )
    {
        st->api->finalize( st );
    }
    return rc;
}


void cson_session_cpdo_finalize( cson_sessmgr * self )
{
    if( self && (self->api == &cson_sessmgr_api_cpdo) )
    {
        cson_sessmgr_cpdo_impl * impl = (cson_sessmgr_cpdo_impl *)self->impl;
        free( impl->table );
        free( impl->fId );
        free( impl->fSession );
        free( impl->fTimestamp );
        if( impl->drv ) impl->drv->api->close( impl->drv );
        free( impl );
        *self = cson_sessmgr_cpdo_empty;
        free( self );
    }
}

int cson_sessmgr_cpdo( cson_sessmgr ** tgt, cson_object const * opt )
{
    if( !tgt || !opt ) return cson_rc.ArgError;
    else
    {
        cson_sessmgr * m = (cson_sessmgr *)malloc(sizeof(cson_sessmgr));
        cson_sessmgr_cpdo_impl * impl = m
            ? (cson_sessmgr_cpdo_impl *)malloc(sizeof(cson_sessmgr_cpdo_impl))
            : NULL;
        cpdo_driver * drv = NULL;
        char const * cstr;
        char const * user;
        char const * password;
        int rc;
        if( ! m ) return cson_rc.AllocError;
        else if( ! impl )
        {
            free(m);
            return cson_rc.AllocError;
        }
#if CPDO_ENABLE_SQLITE3
        cpdo_driver_sqlite3_register();
#endif
#if CPDO_ENABLE_MYSQL5
        cpdo_driver_mysql5_register();
#endif
        
        *impl = cson_sessmgr_cpdo_impl_empty;
        *m = cson_sessmgr_cpdo_empty;
        m->impl = impl;

#define GETSTR(KEY) cson_string_cstr( cson_value_get_string( cson_object_get( opt, KEY ) ) )
        cstr = GETSTR("dsn");
        if( ! cstr || !*cstr )
        {
            rc = cson_rc.ArgError;
            goto error_clean;
        }
        user = GETSTR("user");
        password = GETSTR("password");
        rc = cpdo_driver_new_connect( &drv, cstr, user, password );
        if( rc )
        {
            if( drv )
            {
                drv->api->close( drv );
            }
            /* There are overlaps/discrepancies in the cson_rc and cpdo_rc error
               values, so we're going to convert this to a cson_rc value...
             */
            rc = cson_rc.IOError;
            goto error_clean;
        }
        impl->drv = drv;
#define CP(KEY,KEYF)                                                \
        cstr = GETSTR(KEY);                                          \
        if( ! cstr ) {                                               \
            rc = cson_rc.ArgError;                                   \
            goto error_clean;                                        \
        } else {                                                     \
            impl->KEYF = cson_session_cpdo_strdup( cstr );           \
            if( ! impl->KEYF ) {                                     \
                rc = cson_rc.AllocError;                             \
                goto error_clean;                                    \
            }                                                        \
        } (void)0
        CP("table", table);
        CP("fieldId", fId);
        CP("fieldSession", fSession);
        CP("fieldTimestamp", fTimestamp);
#undef CP
#undef GETSTR
        *tgt = m;
        return 0;
        error_clean:
        m->api->finalize( m );
        return rc;
    }
}

#undef IMPL_DECL
#undef ENABLE_UNIX
#undef MARKER

#endif /* CSON_ENABLE_CPDO */
