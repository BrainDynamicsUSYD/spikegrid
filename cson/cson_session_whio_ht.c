/************************************************************************
A cson_sessmgr implementation which uses whio_ht's on-storage hashtable
for storage.

Possible bugs:

- Storage-level locking code is in place but only mildly tested. If
the underlying storage reports that it doesn't support locking, we
optimistically try to operate without it. i did manage, with a
debugger connected to hold the hashtable open, to get a blocking lock
in a second instance of a test app. So it's been shown to basically
work.

************************************************************************/
#include "wh/cson/cson_session.h"
#include "wh/whio/whio_amalgamation.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

static int cson_session_whio_ht_load( cson_sessmgr * self, cson_value ** tgt, char const * id );
static int cson_session_whio_ht_save( cson_sessmgr * self, cson_value const * root, char const * id );
static int cson_session_whio_ht_remove( cson_sessmgr * self, char const * id );
static void cson_session_whio_ht_finalize( cson_sessmgr * self );

static const cson_sessmgr_api cson_sessmgr_api_whio_ht =
{
    cson_session_whio_ht_load,
    cson_session_whio_ht_save,
    cson_session_whio_ht_remove,
    cson_session_whio_ht_finalize
};

typedef struct cson_sessmgr_whio_ht_impl cson_sessmgr_whio_ht_impl;
struct cson_sessmgr_whio_ht_impl
{
    /**
       Parent device (file) of the hashtable.
    */
    whio_dev * dev;
    /**
       The on-storage hashtable.
     */
    whio_ht ht;
};

static const cson_sessmgr_whio_ht_impl cson_sessmgr_whio_ht_impl_empty = {
NULL/*dev*/,
whio_ht_empty_m/*ht*/
};

static const cson_sessmgr cson_sessmgr_whio_ht_empty =
{
    &cson_sessmgr_api_whio_ht,
    NULL
};

/* Helper macro for various cson_sessmgr_api member implementations. */
#define IMPL_DECL(RC) \
    cson_sessmgr_whio_ht_impl * impl = (self && (self->api == &cson_sessmgr_api_whio_ht)) \
    ? (cson_sessmgr_whio_ht_impl*)self->impl \
    : NULL;                                   \
    if( NULL == impl ) return RC

static int cson_session_whio_ht_dev_lock( whio_dev * dev, char doLock, char write )
{
    whio_lock_request lock = whio_lock_request_empty;
    int rc;
    assert( NULL != dev );
    lock.type = doLock
        ? (write ? whio_lock_TYPE_WRITE : whio_lock_TYPE_READ)
        : whio_lock_TYPE_UNLOCK;
    lock.command = whio_lock_CMD_SET_WAIT;
    lock.start = 0;
    lock.length = 0;
    rc = whio_dev_lock( dev, &lock ); 
    if( (0==rc) || (whio_rc.UnsupportedError==rc) )
    {
        /* assume UnsupportedError is okay. */
        return 0;
    }
    return rc;
}

#define DEV_LOCK(DEV,WRITE) cson_session_whio_ht_dev_lock( (DEV), 1, (WRITE) )
#define DEV_UNLOCK(DEV) cson_session_whio_ht_dev_lock( (DEV), 0, 0 )

static int cson_session_whio_ht_load( cson_sessmgr * self, cson_value ** root, char const * id )
{
    whio_ht_record rec = whio_ht_record_empty;
    void * key = NULL;
    void * value = NULL;
    whio_size_t keyLen = 0;
    whio_size_t valLen = 0;
    int rc;
    IMPL_DECL(cson_rc.ArgError);
    if( ! root || !id || !*id ) return cson_rc.ArgError;
    rc = DEV_LOCK(impl->dev,0);
    if( rc ) return cson_rc.IOError;
    rc = whio_ht_search( &impl->ht, id, strlen(id), &rec );
    if( rc && (whio_rc.NotFoundError != rc) )
    {
        DEV_UNLOCK(impl->dev);
        return cson_rc.IOError;
    }
    rc = whio_ht_kv_get_alloc( &impl->ht, &rec, &key, &keyLen, &value, &valLen );
    DEV_UNLOCK(impl->dev);
    if( rc )
    {
        return (whio_rc.AllocError == rc)
            ? cson_rc.AllocError
            : cson_rc.IOError;
    }
    rc = value
        ? cson_parse_string( root, (char const *)value, strlen((char const *)value), NULL, NULL )
        : cson_rc.RangeError;
    free( key ) /* also frees value! */;
    return rc;
}

static int cson_session_whio_ht_save( cson_sessmgr * self, cson_value const * root, char const * id )
{
    static const cson_output_opt outOpt = cson_output_opt_empty_m;
    cson_buffer buf = cson_buffer_empty;
    int rc;
    size_t idlen;
    IMPL_DECL(cson_rc.ArgError);
    if( !root || !id || !*id ) return cson_rc.ArgError;
    rc = cson_output_buffer( root, &buf, &outOpt );
    if( rc )
    {
        goto end_clean;
    }
    idlen = strlen(id);
    rc = DEV_LOCK(impl->dev,1);
    if( rc ) return cson_rc.IOError;

    rc = whio_ht_remove( &impl->ht, id, idlen );
    if( rc && (whio_rc.NotFoundError != rc) )
    {
        rc = cson_rc.IOError;
        goto end_clean;
    }
    rc = whio_ht_insert( &impl->ht, id, idlen, buf.mem, buf.used );
    end_clean:
    DEV_UNLOCK(impl->dev);
    cson_buffer_reserve( &buf, 0 );
    return rc;
}

void cson_session_whio_ht_finalize( cson_sessmgr * self )
{
    if( self && (self->api == &cson_sessmgr_api_whio_ht) )
    {
        cson_sessmgr_whio_ht_impl * impl = (cson_sessmgr_whio_ht_impl *)self->impl;
        if( impl->dev )
        {
            /* dev is owned by impl->ht. */
        }
        whio_ht_close( &impl->ht );
        free( impl );
        *self = cson_sessmgr_whio_ht_empty;
        free( self );
    }
}

static int cson_session_whio_ht_remove( cson_sessmgr * self, char const * id )
{
    int rc;
    IMPL_DECL(cson_rc.ArgError);
    if( ! id || !*id ) return cson_rc.ArgError;
    rc = DEV_LOCK(impl->dev,1);
    if( rc ) return cson_rc.IOError;
    rc = whio_ht_remove( &impl->ht, id, strlen(id) );
    DEV_UNLOCK(impl->dev);
    if( whio_rc.NotFoundError == rc ) return cson_rc.NotFoundError;
    else if( rc ) return cson_rc.IOError;
    else return 0;
}


int cson_sessmgr_whio_ht( cson_sessmgr ** tgt, cson_object const * opt )
{
    int rc;
    char const * htname = NULL;
    cson_sessmgr * m = NULL;
    cson_sessmgr_whio_ht_impl * impl = NULL;
    if( ! tgt || !opt ) return cson_rc.ArgError;
    htname = cson_string_cstr( cson_value_get_string( cson_object_get( opt, "file" ) ) );
    if( ! htname )
    {
        return cson_rc.ArgError;
    }
    m = (cson_sessmgr *)malloc(sizeof(cson_sessmgr));
    impl = m
        ? (cson_sessmgr_whio_ht_impl *)malloc(sizeof(cson_sessmgr_whio_ht_impl))
        : NULL;
    if( ! impl )
    {
        free(m);
        return cson_rc.AllocError;
    }
    *m = cson_sessmgr_whio_ht_empty;
    *impl = cson_sessmgr_whio_ht_impl_empty;
    m->impl = impl;

    impl->dev = whio_dev_for_filename( htname, "r+" );
    if( ! impl->dev )
    {
        rc = cson_rc.IOError;
        goto error_clean;
    }

    {
        whio_ht * htPtr = NULL;
        htPtr = &impl->ht;
        rc = whio_ht_open( &htPtr, impl->dev );
        if( rc )
        {
            impl->dev->api->finalize( impl->dev );
            impl->dev = NULL;
            rc = cson_rc.ArgError;
            goto error_clean;
        }
    }

#if 0
    {
        cson_string const * jstr;
#define CP(KEY) \
        jstr = cson_value_get_string( cson_object_get( opt, # KEY ) ); \
        if( jstr ) { \
            impl->KEY = cson_session_whio_ht_strdup( cson_string_cstr( jstr ) ); \
            if( ! impl->KEY ) { \
                rc = cson_rc.AllocError;        \
                goto error_clean;               \
            } \
        } (void)0
#undef CP
    }
#define CP(KEY,VAL) if( ! impl->KEY ) { \
            impl->KEY = cson_session_whio_ht_strdup(VAL); \
            if( ! impl->KEY ) { \
                rc = cson_rc.AllocError;        \
                goto error_clean;               \
            } \
        } (void)0
    CP(prefix,"cson-session-");
    CP(suffix,".json");
#undef CP
#endif
    *tgt = m;
    return 0;
    error_clean:
    m->api->finalize( m );
    return rc;
}

#undef IMPL_DECL
#undef DEV_LOCK
#undef DEV_UNLOCK
