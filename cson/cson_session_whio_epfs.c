/************************************************************************
A cson_sessmgr implementation which uses whio_epfs's on-storage hashtable
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

static int cson_session_whio_epfs_load( cson_sessmgr * self, cson_value ** tgt, char const * id );
static int cson_session_whio_epfs_save( cson_sessmgr * self, cson_value const * root, char const * id );
static int cson_session_whio_epfs_remove( cson_sessmgr * self, char const * id );
static void cson_session_whio_epfs_finalize( cson_sessmgr * self );

static const cson_sessmgr_api cson_sessmgr_api_whio_epfs =
{
    cson_session_whio_epfs_load,
    cson_session_whio_epfs_save,
    cson_session_whio_epfs_remove,
    cson_session_whio_epfs_finalize
};

typedef struct cson_sessmgr_whio_epfs_impl cson_sessmgr_whio_epfs_impl;
struct cson_sessmgr_whio_epfs_impl
{
    /**
       The EPFS storage.
     */
    whio_epfs fs;
};

static const cson_sessmgr_whio_epfs_impl cson_sessmgr_whio_epfs_impl_empty = {
whio_epfs_empty_m/*fs*/
};

static const cson_sessmgr cson_sessmgr_whio_epfs_empty =
{
    &cson_sessmgr_api_whio_epfs,
    NULL
};

/* Helper macro for various cson_sessmgr_api member implementations. */
#define IMPL_DECL(RC) \
    cson_sessmgr_whio_epfs_impl * impl = (self && (self->api == &cson_sessmgr_api_whio_epfs)) \
    ? (cson_sessmgr_whio_epfs_impl*)self->impl \
    : NULL;                                   \
    if( NULL == impl ) return RC

static int cson_data_source_whio_dev( void * state, void * dest, unsigned int * n )
{
    whio_dev * dev = (whio_dev*)state;
    whio_size_t rc;
    assert( NULL != dev && dest && n );
    rc = dev->api->read( dev, dest, *n );
    if( rc == *n )
    {
        return 0;
    }
    else
    {
        /* a short read. Possibly EOF. */
        *n = rc;
        return dev->api->eof(dev)
            ? 0
            : cson_rc.IOError;
    }
}


static int cson_session_whio_epfs_load( cson_sessmgr * self, cson_value ** root, char const * id )
{
    int rc;
    size_t idlen;
    whio_epfs_id_t inodeID = 0;
    whio_dev * dev = NULL;
    IMPL_DECL(cson_rc.ArgError);
    if( ! root || !id || !*id ) return cson_rc.ArgError;
    idlen = strlen( id );
    rc = whio_epfs_name_search( &impl->fs, &inodeID,
                                (whio_epfs_namer_const_string)id, idlen );
    if( rc )
    {
        if( whio_rc.NotFoundError == rc )
        {
            rc = cson_rc.NotFoundError;
        }
        else
        {
            rc = cson_rc.IOError;
        }
        return rc;
    }

    rc = whio_epfs_dev_open( &impl->fs, &dev, inodeID, WHIO_MODE_RO );
    if( rc )
    {
        rc = cson_rc.IOError;
        goto end_clean;
    }
    rc = cson_parse( root, cson_data_source_whio_dev, dev, NULL, NULL );
    end_clean:
    if( dev ) dev->api->finalize( dev );
    return rc;
}

static int cson_session_whio_epfs_save( cson_sessmgr * self, cson_value const * root, char const * id )
{
    static const cson_output_opt outOpt = cson_output_opt_empty_m;
    cson_buffer buf = cson_buffer_empty;
    int rc;
    size_t idlen;
    whio_epfs_id_t inodeID = 0;
    whio_dev * dev = NULL;
    char needToSetName = 0;
    IMPL_DECL(cson_rc.ArgError);
    if( !root || !id || !*id ) return cson_rc.ArgError;
    idlen = strlen(id);
    rc = whio_epfs_name_search( &impl->fs, &inodeID,
                                (whio_epfs_namer_const_string)id, idlen );
    if( rc )
    {
        assert( whio_rc.UnsupportedError != rc );
        if( whio_rc.NotFoundError == rc )
        {
            /* okay. Ignore this. */
            rc = 0;
        }
        else
        {
            rc = cson_rc.IOError;
            goto end_clean;
        }
    }
    needToSetName = inodeID ? 0 : 1;
    rc = cson_output_buffer( root, &buf, &outOpt );
    if( rc )
    {
        goto end_clean;
    }
    rc = whio_epfs_dev_open( &impl->fs, &dev, inodeID, WHIO_MODE_RWC );
    if( rc )
    {
        rc = cson_rc.IOError;
        goto end_clean;
    }
    inodeID = whio_epfs_dev_inode_id( dev );
    assert( 0 != inodeID );
    if( needToSetName )
    {
        rc = whio_epfs_name_set( &impl->fs, inodeID, (whio_epfs_namer_const_string)id,
                                 idlen );
        assert( whio_rc.UnsupportedError != rc );
        if( 0 != rc )
        {
            rc = cson_rc.IOError;
            goto end_clean;
        }
    }
    rc = dev->api->truncate( dev, buf.used );
    if( 0 != rc )
    {
        rc = cson_rc.IOError;
        goto end_clean;
    }
    if( 0 != dev->api->seek( dev, 0, SEEK_SET ) )
    {
        rc = cson_rc.IOError;
        goto end_clean;
    }
    if( buf.used != dev->api->write( dev, buf.mem, buf.used ) )
    {
        rc = cson_rc.IOError;
        goto end_clean;
    }

    end_clean:
    if( dev ) dev->api->finalize(dev);
    whio_epfs_flush( &impl->fs );
    cson_buffer_reserve( &buf, 0 );
    return rc;
}

void cson_session_whio_epfs_finalize( cson_sessmgr * self )
{
    if( self && (self->api == &cson_sessmgr_api_whio_epfs) )
    {
        cson_sessmgr_whio_epfs_impl * impl = (cson_sessmgr_whio_epfs_impl *)self->impl;
        whio_epfs_close( &impl->fs );
        free( impl );
        *self = cson_sessmgr_whio_epfs_empty;
        free( self );
    }
}

static int cson_session_whio_epfs_remove( cson_sessmgr * self, char const * id )
{
    int rc;
    whio_epfs_id_t inodeID = 0;
    unsigned int idlen;
    IMPL_DECL(cson_rc.ArgError);
    if( !id || !*id ) return cson_rc.ArgError;
    idlen = strlen(id);
    rc = whio_epfs_name_search( &impl->fs, &inodeID,
                                (whio_epfs_namer_const_string)id, idlen );
    if( rc )
    {
        assert( whio_rc.UnsupportedError != rc );
        if( whio_rc.NotFoundError == rc )
        {
            /* okay. Ignore this. */
            return 0;
        }
        else
        {
            return cson_rc.IOError;
        }
    }
    else
    {
        return whio_epfs_unlink( &impl->fs, inodeID )
            ? cson_rc.IOError
            : 0;
    }
}


int cson_sessmgr_whio_epfs( cson_sessmgr ** tgt, cson_object const * opt )
{
    char const * fname = NULL;
    if( ! tgt || !opt ) return cson_rc.ArgError;
    fname = cson_string_cstr( cson_value_get_string( cson_object_get( opt, "file" ) ) );
    if( ! fname )
    {
        return cson_rc.ArgError;
    }
    else
    {
        int rc;
        cson_sessmgr * m = NULL;
        cson_sessmgr_whio_epfs_impl * impl = NULL;
        whio_epfs_setup_opt opt = whio_epfs_setup_opt_empty;
        opt.storage.dev = whio_dev_for_filename( fname, "r+" );
        if( !opt.storage.dev )
        {
            return cson_rc.IOError;
        }
        opt.storage.takeDevOnSuccess = true;
        opt.storage.enableLocking = true;
        m = (cson_sessmgr *)malloc(sizeof(cson_sessmgr));
        impl = m
            ? (cson_sessmgr_whio_epfs_impl *)malloc(sizeof(cson_sessmgr_whio_epfs_impl))
            : NULL;
        if( ! impl )
        {
            free(m);
            opt.storage.dev->api->finalize( opt.storage.dev );
            return cson_rc.AllocError;
        }
        *m = cson_sessmgr_whio_epfs_empty;
        *impl = cson_sessmgr_whio_epfs_impl_empty;
        m->impl = impl;
        {
            whio_epfs * fsPtr = &impl->fs;
            rc = whio_epfs_openfs( &fsPtr, &opt );
        }
        if( rc )
        {
            rc = cson_rc.IOError;
            goto end_clean;
        }
        assert( NULL == opt.storage.dev );
        if( ! whio_epfs_has_namer( &impl->fs ) )
        {
            /* we can't work with this fs. */
            rc = cson_rc.ArgError;
            goto end_clean;
        }
        *tgt = m;
        return 0 /* reminder: impl->fs owns opt.storage.dev now */;
        end_clean:
        if( opt.storage.dev )
        {
            opt.storage.dev->api->finalize( opt.storage.dev );
            m->api->finalize( m );
        }
        return rc;
    }
}

#undef IMPL_DECL
