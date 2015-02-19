#if !defined(WANDERINGHORSE_NET_CSON_SESSION_H_INCLUDED)
#define WANDERINGHORSE_NET_CSON_SESSION_H_INCLUDED 1

/** @page page_cson_session cson Session API

The cson_session API provides a small interface,
called cson_sessmgr, which defines the basic operations
needed for implementent persistent application state,
across application sessions, by storing the state as
JSON data in "some back-end storage." The exact underlying
storage is not specified by the interface, but two
implementations are provided by the library:

- File-based sessions.

- Database-based sessions, using libcpdo for connection
abstraction.

libcpdo is included, in full, in the cson source tree,
but can also be found on its web page:

    http://fossil.wanderinghorse.net/repos/cpdo/

@see cson_sessmgr_register()
@see cson_sessmgr_load()
@see cson_sessmgr_names()
@see cson_sessmgr
@see cson_sessmgr_api
*/

#include "wh/cson/cson.h"

#if defined(__cplusplus)
extern "C" {
#endif

    typedef struct cson_sessmgr cson_sessmgr;
    typedef struct cson_sessmgr_api cson_sessmgr_api;

    /** @struct cson_sessmgr_api
        
       Defines operations required by "session managers." Session managers
       are responsible for loading and saving cson session information
       in the form of JSON data.

       @see cson_sessmgr
    */
    struct cson_sessmgr_api
    {
        /**
           Loads/creates a session object (JSON data). The
           implementation must use the given identifier for loading an
           existing session, creating the session if createIfNeeded is
           true and the session data is not found. If createIfNeeded
           is true then the implementation must create an Object for
           the session root, as opposed to an Array or other JSON
           value. Clients are allowed to use non-Objects as their
           sessions but doing so would seem to have no benefit, and is
           not recommended.

           If the given id cannot be found then cson_rc.NotFoundError
           must be returned. On success it must assign the root node
           of the session tree to *tgt and return 0.  On error *tgt
           must not be modified and non-zero must be returned.

           On success ownership of *tgt is transfered to the caller.

           Error conditions include:

           - self, tgt, or id are NULL: cson_rc.ArgError

           - id is "not valid" (the meaning of "valid" is
           implementation-dependent): cson_rc.ArgError

           The identifier string must be NUL-terminated. Its maximum
           length, if any, is implementation-dependent.
        */
        int (*load)( cson_sessmgr * self, cson_value ** tgt, char const * id );

        /**
           Must save the given JSON object tree to the underlying storage, using the given identifier
           as its unique key. It must overwrite any existing session with that same identifier.
        */
        int (*save)( cson_sessmgr * self, cson_value const * root, char const * id );

        /**
           Must remove all session data associated with the given id.

           Must return 0 on success, non-0 on error.
        */
        int (*remove)( cson_sessmgr * self, char const * id );
        /**
           Must free up any resources used by the self object and then
           free self. After calling this, further use of the self
           object invokes undefined behaviour.
        */
        void (*finalize)( cson_sessmgr * self );
    };

    /**
       cson_sessmgr is base interface type for concrete
       cson_sessmgr_api implementations.  Each holds a pointer to its
       underlying implementation and to implementation-private
       data.
    
       @see cson_sessmgr_register()
       @see cson_sessmgr_load()
       @see cson_sessmgr_names()
       @see cson_sessmgr_api
    */
    struct cson_sessmgr
    {
        /**
           The concrete implementation functions for this
           session manager instance.
        */
        const cson_sessmgr_api * api;
        /**
           Private implementation date for this session manager
           instance. It is owned by this object and will be freed when
           thisObject->api->finalize(thisObject) is called. Client
           code must never use nor rely on the type/contents of the
           memory stored here.
        */
        void * impl;
    };

    /**
       A typedef for factory functions which instantiate cson_sessmgr
       instances.

       The semantics are:

       - tgt must be a non-NULL pointer where the result object can be
       stored. If it is NULL, cson_rc.ArgError must be returned.

       - opt (configuration options) may or may not be required,
       depending on the manager. If it is required and not passed in,
       cson_rc.ArgError must be returned. If the config options are
       required but the passed-in object is missing certain values, or
       has incorrect values, the implementation may substitute
       sensible defaults (if possible) or return cson_rc.ArgError.

       - On error non-0 (one of the cson_rc values) must be returned
       and tgt must not be modified.

       - On success *tgt must be pointed to the new manager object,
       zero must be returned, and the caller takes over ownership of
       the *tgt value (and must eventually free it with
       obj->api->finalize(obj)).
    */
    typedef int (*cson_sessmgr_factory_f)( cson_sessmgr ** tgt, cson_object const * config );
    
#define cson_sessmgr_empty_m { NULL/*api*/, NULL/*impl*/ }

    /**
       Registers a session manager by name. The given name must be a
       NUL-terminaed string shorter than some internal limit
       (currently 32 bytes, including the trailing NUL). f must be a
       function conforming to the cson_sessmgr_factory_f() interface.

       On success returns 0.

       On error either one of the arguments was invalid, an entry with
       the given name was already found, or no space is left in the
       internal registration list. The API guarantees that at least 10
       slots are initially available, and it is not anticipated that
       more than a small handful of them will ever be used.

       This function is not threadsafe - do not register factories
       concurrently from multiple threads.

       By default the following registrations are (possibly)
       pre-installed:

       - "file" = cson_sessmgr_file()

       - "cpdo" = cson_sessmgr_cpdo() IF this library is compiled with
       the macro CSON_ENABLE_CPDO set to a true value. Exactly which
       databases are supported by that back-end (if any) are
       determined by how the cpdo library code is compiled.

       - "whio_ht" = cson_sessmgr_whio_ht() IF this library is compiled
       with whio support.

       - "whio_epfs" = cson_sessmgr_whio_epfs() IF this library is
       compiled with whio support.
    */
    int cson_sessmgr_register( char const * name, cson_sessmgr_factory_f f );

    /**
       A front-end to loading cson_sessmgr intances by their
       cson_session-conventional name. The first arguments must be a
       NUL-terminated string holding the name of the session manager
       driver. The other two arguments have the same semantics as for
       cson_sessmgr_factory_f(), so see that typedef's documentation
       regarding, e.g., ownership of the *tgt value.

       This function is thread-safe with regards to itself but not
       with regards to cson_sessmgr_register(). That is, it is legal
       to call this function concurrently from multiple threads,
       provided the arguments themselves are not being used
       concurrently. However, it is not safe to call this function
       when cson_sessmgr_register() is being called from another
       thread, as that function modifies the lookup table used by this
       function.

       On success 0 is returned and the ownership of *tgt is as
       documented for cson_sessmgr_factory_f(). On error non-0 is
       returned and tgt is not modified.
    */
    int cson_sessmgr_load( char const * name, cson_sessmgr ** tgt, cson_object const * opt );

    /**
       Returns the list of session managers registered via
       cson_sessmgr_register(). This function is not thread-safe in
       conjunction with cson_sessmgr_register(), and results are
       undefined if that function is called while this function is
       called or the results of this function call are being used.

       The returned array is never NULL but has a NULL as its final
       entry.

       Example usage:

       @code
       char const * const * mgr = cson_sessmgr_names();
       for( ; *mgr; ++mgr ) puts( *mgr );
       @endcode
    */
    char const * const * cson_sessmgr_names();
    
    /**
       A cson_sessmgr_factory_f() implementation which returns a new
       session manager which uses local files for storage.

       tgt must be a non-NULL pointer where the result can be stored.
       
       The opt object may be NULL or may be a configuration object
       with the following structure:

       @code
       {
       dir: string (directory to store session files in),
       prefix: string (prefix part of filename),
       suffix: string (file extension, including leading '.')
       }
       @endcode

       Any missing options will assume (unspecified) default values.
       This routine does not ensure the validity of the option values,
       other than to make sure they are strings.

       The returned object is owned by the caller, who must eventually
       free it using obj->api->finalize(obj). If it returns NULL,
       the error was an out-of-memory condition or tgt was NULL.

       On error non-0 is returned, but the only error conditions are
       allocation errors and (NULL==tgt), which will return
       cson_rc.AllocError resp. cson_rc.ArgError.

       Threading notes:

       - As long as no two operations on these manager instances use
       the same JSON object and/or session ID at the same time,
       multi-threaded usage should be okay. All save()/load()/remove()
       data is local to those operations, with the exception of the
       input arguments (which must not be used concurrently to those
       calls).

       Storage locking:

       - No locking of input/output files is done, under the
       assumption that only one thread/process will be using a given
       session ID (which should, after all, be unique world-wide).  If
       sessions will only be read, not written, there is little danger
       of something going wrong vis-a-vis locking (assuming the
       session files exists and can be read).

       TODO:

       - Add a config option to enable storage locking. If we'll
       re-implement this to use the whio API under the hood then we
       could use the (slightly simpler) whio_lock API for this.
    */
    int cson_sessmgr_file( cson_sessmgr ** tgt, cson_object const * opt );

    /**
       This is only available if cson is compiled with cpdo support.

       Implements the cson_sessmgr_factory_f() interface.
       
       This function tries to create a database connection using the options
       supplied in the opt object. The opt object must structurarly look like:

       @code
       {
       "dsn": "cpdo dsn string",
       "user": "string",
       "password": "string",
       "table": "table_name_where_sessions_are_stored",
       "fieldId": "field_name_for_session_id (VARCHAR/STRING)",
       "fieldTimestamp": "field_name_for_last_saved_timestamp (INTEGER)",
       "fieldSession": "field_name_for_session_data (TEXT)"
       }
       @endcode

       On success it returns 0 and sets *tgt to the new session manager,
       which is owned by the caller and must eventually be freed by calling
       obj->api->finalize(obj).

       This function can fail for any number of reasons:

       - Any parameters are NULL (cson_rc.ArgError).

       - cpdo cannot connect to the given DSN with the given
       username/password. Any error in establishing a connection causes
       cson_rc.IOError to be returned, as opposed to the underlying
       cpdo error code.

       - Any of the "table" or "fieldXXX" properties are NULL. It
       needs these data in order to know where to load/save sessions.


       If any required options are missing, cson_rc.ArgError is
       returned.

       TODO: add option "preferBlob", which can be used to set the db
       field type preference for the fieldSession field to
       blob. Currently it prefers string but will try blob operations
       if string ops fail.  Blobs have the disadvantage of much larger
       encoded sizes but the advantage that the JSON data is encoded
       (at least by sqlite3) as a hex number stream, making it
       unreadable to casual observers.       

       @endcode
    */
    int cson_sessmgr_cpdo( cson_sessmgr ** tgt, cson_object const * opt );

    /**
       This cson_sessmgr_factory_f() implementation might or might not
       be compiled in, depending on the mood of the cson
       maintainer. It is very niche-market, and primarily exists just
       to test (and show off) the whio_ht code.

       It uses libwhio's on-storage hashtable (called whio_ht)
       as the underlying storage:
       
       http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_ht

       The opt object must not be NULL and must contain a single
       string property named "file" which contains the path to the
       whio_ht file to use for sessions. That file must have been
       previously created, either programatically using the whio_ht
       API or using whio-ht-tool:

       http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_ht_tool

       See cson_sessmgr_factory_f() for the semantics of the tgt
       argument and the return value.

       Threading notes:

       While the underlying hashtable supports a client-defined mutex,
       this usage of it does not set one (because we have no default
       one to use). What this means for clients is that they must not
       use this session manager from multiple threads, nor may they
       use multiple instances in the same process which use the same
       underlying hashtable file from multiple threads. How best to
       remedy this (allowing the client to tell this API what mutex to
       use) is not yet clear. Maybe a global whio_mutex object which
       the client must initialize before instantiating these session
       managers.
       
       Storage Locking:

       If the underlying filesystem reports that it supports file
       locking (via the whio_lock API, basically meaning POSIX
       fcntl()-style locking) the the session manager will use it. For
       the load() operation a read lock is acquired and for
       save()/remove() a write lock. The operations will fail if
       locking fails, the exception being if the device reports that
       it doesn't support locking, in which case we optimistically
       save/load/remove without locking.

       Remember that in POSIX-style locking, a single process does not
       see its own locks and can overwrite locks set via other
       threads. This means that multi-threaded use of a given
       instance, or multiple instances in the same process using the
       same underlying hashtable file, will likely eventually corrupt
       the hashtable.

       TODO:

       - Add a config option to disable storage locking, for clients
       who really don't want to use it.
    */
    int cson_sessmgr_whio_ht( cson_sessmgr ** tgt, cson_object const * opt );

    /**
       This cson_sessmgr_factory_f() implementation might or might not
       be compiled in, depending on the mood of the cson
       maintainer. It is very niche-market, and primarily exists just
       to test (and show off) the whio_epfs code.

       It uses libwhio's embedded filesystem (called whio_epfs) as the
       underlying storage:
       
       http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs

       The opt object must not be NULL and must contain a single
       string property named "file" which contains the path to the
       whio_epfs "container file" to use for storing sessions. That
       file must have been previously created, either programatically
       using the whio_epfs API or using whio-epfs-mkfs:

       http://fossil.wanderinghorse.net/repos/whio/index.cgi/wiki/whio_epfs_mkfs

       The EPFS container file MUST be created with a "namer"
       installed. See the above page for full details and examples.
       
       See cson_sessmgr_factory_f() for the semantics of the tgt
       argument and the return value.

       Threading notes:

       - It is not legal to use this session manager from multiple threads.
       Doing so will eventually corrupt the underlying EFS if multiple writers
       work concurrently, and will also eventually _appear_ corrupt to multiple
       readers.

       Storage locking:

       The underlying storage (EFS container file) is locked (with a
       write lock) for the lifetime the the returned session manager
       IF the storage reports that it supports locking. Unlocked
       write access from an outside application will corrupt the EFS.

       TODOs:

       - Add config option to explicitly disable locking support.
    */
    int cson_sessmgr_whio_epfs( cson_sessmgr ** tgt, cson_object const * opt );
    
#if 0
    /** TODO? dummy manager which has no i/o support. */
    int cson_sessmgr_transient( cson_sessmgr ** tgt );
#endif

/* LICENSE

This software's source code, including accompanying documentation and
demonstration applications, are licensed under the following
conditions...

Certain files are imported from external projects and have their own
licensing terms. Namely, the JSON_parser.* files. See their files for
their official licenses, but the summary is "do what you want [with
them] but leave the license text and copyright in place."

The author (Stephan G. Beal [http://wanderinghorse.net/home/stephan/])
explicitly disclaims copyright in all jurisdictions which recognize
such a disclaimer. In such jurisdictions, this software is released
into the Public Domain.

In jurisdictions which do not recognize Public Domain property
(e.g. Germany as of 2011), this software is Copyright (c) 2011 by
Stephan G. Beal, and is released under the terms of the MIT License
(see below).

In jurisdictions which recognize Public Domain property, the user of
this software may choose to accept it either as 1) Public Domain, 2)
under the conditions of the MIT License (see below), or 3) under the
terms of dual Public Domain/MIT License conditions described here, as
they choose.

The MIT License is about as close to Public Domain as a license can
get, and is described in clear, concise terms at:

    http://en.wikipedia.org/wiki/MIT_License

The full text of the MIT License follows:

--
Copyright (c) 2011 Stephan G. Beal (http://wanderinghorse.net/home/stephan/)

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.

--END OF MIT LICENSE--

For purposes of the above license, the term "Software" includes
documentation and demonstration source code which accompanies
this software. ("Accompanies" = is contained in the Software's
primary public source code repository.)

*/

#if defined(__cplusplus)
} /*extern "C"*/
#endif

#endif /* WANDERINGHORSE_NET_CSON_SESSION_H_INCLUDED */
