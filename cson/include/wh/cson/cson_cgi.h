#if !defined(WANDERINGHORSE_NET_CSON_CGI_H_INCLUDED)
#define WANDERINGHORSE_NET_CSON_CGI_H_INCLUDED 1

/** @page page_cson_cgi cson CGI API

cson_cgi is a small framework encapsulating features usefor for
writing JSON-only applications (primarily CGI apps) in C. It is based
off of the cson JSON library:

    http://fossil.wanderinghorse.net/repos/cson/

In essence, it takes care of the basic CGI-related app setup, making
the data somewhat more accessible for client purposes.  Clients
create, as output, a single JSON object. The framework takes care of
outputing it, along with any necessary HTTP headers.


Primary features:

- Uses cson for its JSON handling, so it's pretty simple to use.

- Provides a simple-to-use mini-framework for writing CGI applications
which generate only JSON output.

- Various sources of system data are converted by the framework to
JSON for use by the client. This includes HTTP GET, OS environment,
command-line arguments, HTTP cookies, and (with some limitations) HTTP
POST.

- Can read unencoded JSON POST data (TODO: read in form-urlencoded as
a JSON object).

- Supports an optional JSON config file.

- Optional persistent sessions using files, sqlite3, or MySQL5
for the storage.


Primary misfeatures:

- Very young and not yet complete.

- Intended for writing apps which ONLY generate JSON data, not HTML.

- JSONP output support is currently incomplete.

- TODO: support reading of POSTed Array data (currently only Objects
work).

- We're missing a good number of convenience functions.

- Add client API for setting cookies. Currently they can be fetched
or removed but not explicitly set.


Other potential TODOs:

- Session support using session cookies for the IDs. For this to work
we also need to add a storage back-end or two (files, db (e.g. using
cpdo), etc.) and have a fast, good source of random numbers for
generating UUIDs. It _seems_ that using the address of (extern char **
environ) as a seed might be fairly random, but there are probably
environments where that won't suffice. i want to avoid
platform-specific bits, like /dev/urandom, if possible.

- Add client-definable i/o routines. We have this code in the whprintf
tree, but i was hoping to avoid having to import that here. This would
allow a client to add, e.g., gzip support.

*/

/** @page page_cson_cgi_session cson CGI Sessions

cson_cgi_init() will initialize a persistent session if it can figure
out everything it needs in order to do so. If it cannot it will simply
skip session initialization. A client can tell if a session was
established or not by calling cson_cgi_get_env_val(cx,'s',0).  If that
returns NULL then no session was created during initialization.
The API currently provides no way to initialize one after the fact.
That is, client code can use cson_cgi_get_env_val(cx,'s',1) to create a
session object, but it won't automatically be persistent across
application sessions.

Session management is really just the following:

- Load a JSON object from some persistent storage.
- Save that object at shutdown.

"Persistent storage" is the important phrase here. How the sessions
are saved is really unimportant. The library uses a generic interface
(cson_sessmgr) to handle the i/o, and can use any implementation we
care to provide. As of this writing (20110413) files, sqlite3, and
MySQL5 are supported.

The session object is a JSON object available via cson_cgi_getenv(),
using "s" as the environment name (the second parameter to
cson_cgi_getenv()).

At library shutdown (typically when main() exits, but also via
cson_cgi_cx_clean()), the session is saved using the current
session manager. If the session is not loaded during initialization,
but is created later, the library will assign a new session ID to it
before saving.

See cson_cgi_config_file() for examples of configuring the session
management.
*/

#include "wh/cson/cson.h"
#include "wh/cson/cson_session.h"

#if defined(__cplusplus)
extern "C" {
#endif

    /** @def CSON_CGI_ENABLE_POST_FORM_URLENCODED

    If CSON_CGI_ENABLE_POST_FORM_URLENCODED is set to a true value
    then the API will try to process form-urlencoded POST data,
    otherwise it will not.

    Reminder to self: this is basically a quick hack for fossil
    integration. We disable form encoding in that build because fossil
    handles that itself and we must not interfere with it.
    */
#if !defined(CSON_CGI_ENABLE_POST_FORM_URLENCODED)
#  define CSON_CGI_ENABLE_POST_FORM_URLENCODED 1
#endif    
    /** @def CSON_CGI_GETENV_DEFAULT

        The default environment name(s) to use for cson_cgi_getenv().
    */
#define CSON_CGI_GETENV_DEFAULT "gpce"
    /** @def CSON_CGI_KEY_JSONP

        TODO?: get rid of this and move cson_cgi_keys into the public
        API?

        The default environment key name to use for checking whether a
        response should used JSONP or not.
    */
#define CSON_CGI_KEY_JSONP "jspon"

#define CSON_CGI_KEY_SESSION "CSONSESSID"


    typedef struct cson_cgi_init_opt cson_cgi_init_opt;
    /** @struct cson_cgi_init_opt

        A type to hold core runtime configuration information for
        cson_cgi.
    */
    struct cson_cgi_init_opt
    {
        /**
           stdin stream. If NULL, stdin is used.
        */
        FILE * inStream;
        /**
           stdout stream. If NULL, stdout is used.
        */
        FILE * outStream;
        /**
           stderr stream. If NULL, stderr is used.
         */
        FILE * errStream;
        /**
           Path to a JSON config file.
        */
        char const * configFile;

        /**
           Alternately, configuration data can be supplied in the form
           of an Object (ownership is transfered to cson_cgi).
        */
        cson_object * configObj;

        /**
           If set then the session will be forced to use this
           ID, otherwise one will be generated.
        */
        char const * sessionID;

        /**
           Values are interpretted as:

           0 = do not output headers when cson_cgi_response_output_all()
           is called.

           (>0) = always output headers when cson_cgi_response_output_all()
           is called.

           (<0) = Try to determine, based on environment variables, if
           we are running in CGI mode. If so, output the headers when
           cson_cgi_response_output_all() is called, otherwise omit
           them.

           If the headers are omitted then so is the empty line
           which normally separates them from the response body!

           The intention of this flag is to allow non-CGI apps
           to disable output of the HTTP headers.
        */
        char httpHeadersMode;
        
        /**
           JSON output options.
        */
        cson_output_opt outOpt;

    };

    /** Empty-initialized cson_cgi_init_opt object. */
#define cson_cgi_init_opt_empty_m {                             \
        NULL/*inStream*/, NULL/*outStream*/, NULL/*errStream*/, \
            NULL /*configFile*/, NULL /*configObj*/, NULL /*sessionID*/, \
            -1/*httpHeadersMode*/,                              \
            cson_output_opt_empty_m /*outOpt*/                  \
    }

    /** Empty-initialized cson_cgi_init_opt object. */
    extern const cson_cgi_init_opt cson_cgi_init_opt_empty;


    /** @struct cson_cgi_env_map
       Holds a cson_object and its cson_value parent
       reference.
    */
    struct cson_cgi_env_map
    {
        /** Parent reference of jobj. */
        cson_value * jval;
        /** Object reference of jval. */
        cson_object * jobj;
    };
    typedef struct cson_cgi_env_map cson_cgi_env_map;

    /** Empty cson_cgi_env_map object. */
#define cson_cgi_env_map_empty_m { NULL, NULL }

    /** @struct cson_cgi_cx

       Internal state used by cson_cgi.

       Clients must not rely on its internal structure. It is in the
       public API so that it can be stack- or custom-allocated.  To
       properly initialize such an object, use cson_cgi_cx_empty_m
       cson_cgi_cx_empty, depending on the context.
    */
    struct cson_cgi_cx
    {
        /**
           Various key/value stores used by the framework. Each
           corresponds to some convention source of key/value
           pairs, e.g. QUERY_STRING or POST parameters.
        */
        struct {
            /**
               The system's environment variables.
            */
            cson_cgi_env_map env;
            /**
               Holds QUERY_STRING key/value pairs.
            */
            cson_cgi_env_map get;
            /**
               Holds POST form/JSON data key/value pairs.
            */
            cson_cgi_env_map post;
            /**
               Holds cookie key/value pairs.
            */
            cson_cgi_env_map cookie;
            /**
               Holds request headers.
            */
            cson_cgi_env_map headers;
        } request;
        /**
           Holds data related to the response JSON.
        */
        struct {
            /**
               HTTP error code to report.
            */
            int httpCode;

            /**
               Root JSON object. Must be an Object or Array (per the JSON
               spec).
            */
            cson_value * root;
            /**
               Holds HTTP response headers as an array of key/value
               pairs.
            */
            cson_cgi_env_map headers;
        } response;
        /**
           A place to store cson_value references
           for cleanup purposes.
        */
        cson_cgi_env_map gc;
        /**
           Holds client-defined key/value pairs.
        */
        cson_cgi_env_map clientEnv;
        cson_cgi_env_map config;
        struct {
            cson_cgi_env_map env;
            cson_sessmgr * mgr;
            char * id;
        } session;
        struct {
            cson_array * jarr;
            cson_value * jval;
        } argv;

        cson_cgi_init_opt opt;
        cson_buffer tmpBuf;
        struct {
            char isJSONP;
            void const * allocStamp;
        } misc;
    };
    typedef struct cson_cgi_cx cson_cgi_cx;
    /**
       Empty-initialized cson_cgi_cx object.
     */
    extern const cson_cgi_cx cson_cgi_cx_empty;
    /**
       Empty-initialized cson_cgi_cx object.
     */
#define cson_cgi_cx_empty_m \
    { \
    { /*maps*/ \
        cson_cgi_env_map_empty_m /*env*/, \
        cson_cgi_env_map_empty_m /*get*/, \
        cson_cgi_env_map_empty_m /*post*/, \
        cson_cgi_env_map_empty_m /*cookie*/, \
        cson_cgi_env_map_empty_m /*headers*/ \
    }, \
    {/*response*/ \
        0 /*httpCode*/, \
        NULL /*root*/, \
        cson_cgi_env_map_empty_m /*headers*/ \
    }, \
    cson_cgi_env_map_empty_m /*gc*/, \
    cson_cgi_env_map_empty_m /*clientEnv*/, \
    cson_cgi_env_map_empty_m /*config*/, \
    {/*session*/ \
        cson_cgi_env_map_empty_m /*env*/, \
        NULL /*mgr*/, \
        NULL /*id*/ \
    }, \
    {/*argv*/ \
        NULL /*jarr*/, \
        NULL /*jval*/ \
    }, \
    cson_cgi_init_opt_empty_m /*opt*/, \
    cson_buffer_empty_m /* tmpBuf */, \
    {/*misc*/ \
        -1 /*isJSONP*/,                         \
            NULL/*allocStamp*/ \
    } \
    }

    cson_cgi_cx * cson_cgi_cx_alloc();
    /**
       Cleans up all internal state of cx. IFF cx was allocated by
       cson_cgi_cx_alloc() then cx is also free()d, else it is assumed
       to have been allocated by the caller (possibly on the stack).

       Returns 1 if cx is not NULL and this function actually frees
       it.  If it returns 0 then either cx is NULL or this function
       cleaned up its internals but did not free(cx) (cx is assumed to
       have been allocated by the client).
    */
    char cson_cgi_cx_clean(cson_cgi_cx * cx);
    
    /**
       Initializes the internal cson_cgi environment and must be
       called one time, from main(), at application startup.

       cx must be either:

       - Created via cson_cgi_cx_alloc().

       - Alternately allocated (e.g. on the stack) and initialized
       by copying cson_cgi_cx_empty over it.

       Any other initialization leads to undefined behaviour.
       
       Returns 0 on success. If it returns non-0 then the rest of the
       API will "almost certainly" not work (and using the rest of the
       API invokes undefined behaviour unless documented otherwise for
       a specific function), so the application should exit
       immediately with an error code. The error codes returned by
       this function all come from the cson_rc object.

       The cx object must eventually be passed to cson_cgi_cx_clean(),
       regardless of success or failure, to clean up any resources
       allocated for the object.

       On success:

       - 0 is returned.

       - The cx object takes over ownership of any streams set in the
       opt object UNLESS they are the stdin/stdout/stderr streams (in
       which case ownership does not change).

       On error non-0 is returned and ownership of the opt.xxStream
       STILL transfers over to cx as described above (because this
       simpifies client-side error handling ).
       
       The 'opt' parameter can be used to tweak certain properties
       of the framework. It may be NULL, in which case defaults are
       used.
       
       This function currently performs the following initialization:

       - Parses QUERY_STRING environment variable into a JSON object.

       - Parses the HTTP_COOKIE environment variable into a JSON object.
       
       - Transforms the system environment to JSON.

       - Copies the list of arguments (argv, in the form conventional
       for main()) to JSON array form for downstream use by the client
       application. It does not interpret these arguments in any
       way. Clients may use cson_cgi_argv() and
       cson_cgi_argv_array() to fetch the list later on in the
       application's lifetime (presumably outside of main()). It is
       legal to pass (argv==NULL) only if argc is 0 or less.

       - If the CONTENT_TYPE env var is one of (application/json,
       application/javascript, or text/plain) and CONTENT_LENGTH is
       set then stdin is assumed to be JSON data coming in via POST.
       An error during that parsing is ignored for initialization purposes
       unless it is an allocation error, in which case it is propagated
       back to the caller of this function.

       - If session management is properly configured in the
       configuration file and if a variable named CSON_CGI_KEY_SESSION
       is found in the environment (cookies, GET, POST, or system env)
       then the previous session is loaded. If it cannot be loaded,
       the error is ignored. (Note that the cookie name can be
       changed via the configuration file.)

       Handling of configuration data:

       This routine tries to pull in configuration data from either a cson
       Object or a file...

       - If options.configObj is set then this function attempts to
       take over ownership of that object. If, upon returning,
       options.configObj is NULL then this function took over
       ownership, else ownership still lies with the caller.

       - If options.configObj is NULL then options.configFile is checked,
       and if it is not NULL then the given file is read in.
       
       - If still no configuration data has been set and the
       CSON_CGI_CONFIG env var is set then that file is read.

       Errors in loading the config from a file are silently ignored.

       TODOs:
       
       - Only read POST data when REQUEST_METHOD==POST?
       
       - Convert form-urlencoded POST data to a JSON object.

       - Potentially add an option to do automatic data type detection
       for numeric GET/POST/ENV/COOKIE data, such that fetching the
       cson_value for such a key would return a numeric value object
       as opposed to a string. Or we could add that option in a
       separate function which walks a JSON Object and performs that
       check/transformation on all of its entries. That currently
       can't be done properly with the cson_object_iterator API
       because changes to the object while looping invalidate the
       iterator. This option would also open up problems when clients
       pass huge strings which just happen to look like numbers.


       @see cson_cgi_config_file()
    */
    int cson_cgi_init( cson_cgi_cx * cx, int argc, char const * const * argv, cson_cgi_init_opt * options );

    /**
       Searches for a value from the CGI environment. The fromWhere
       parameter is a NUL-terminated string which specifies which
       environment(s) to check, and may be made up of any of the
       letters [gprecl], case-insensitive. If fromWhere is NULL or its
       first byte is NUL (i.e. it is empty) then the default value
       defined in CSON_CGI_GETENV_DEFAULT is used.

       The environments are searched in the order specified in
       fromWhere. The various letters mean:

       - g = GET: key/value pairs parsed from the QUERY_STRING
       environment variable.

       - p = POST: form-encoded key/value pairs parsed from stdin.

       - r = REQUEST, equivalent to "gpc", a superset of GET/POST/COOKIE.

       - e = ENV, e.g. via getenv(), but see cson_cgi_env_get_val()
       for more details.

       - c = COOKIE: request cookies (not response cookies) parsed
       from the HTTP_COOKIE environment variable.

       - a = APP: an environment namespace reserved for client app use.

       - f = CONFIG FILE.

       - Use key 's' for the SESSION.
       
       Invalid characters are ignored.

       The returned value is owned by the cson_cgi environment and
       must not be destroyed by the caller. NULL is returned if none
       of the requested environments contain the given key.

       Results are undefined if fromWhere is not NULL and is not
       NUL-terminated.

       TODOs:

       - Replace CSON_CGI_GETENV_DEFAULT with a runtime-configurable
       value (via a config file).

    */
    cson_value * cson_cgi_getenv( cson_cgi_cx * cx, char const * fromWhere, char const * key );

    /**
       A convenience form of cson_cgi_getenv() which returns the given
       key as a string. This will return NULL if the requested key
       is-not-a string value. It does not convert non-string values to
       strings.

       On success the string value is returned. Its bytes are owned by
       this API and are valid until the given key is removed/replaced
       from/in the environment object it was found in or that
       environment object is cleaned up.
    */
    char const * cson_cgi_getenv_cstr( cson_cgi_cx * cx, char const * where, char const * key );

    /**
       During initialization, if the PATH_INFO environment variable is set,
       it is split on '/' characters into array. That array is stored in the
       environment with the name PATH_INFO_SPLIT. This function returns the
       element of the PATH_INFO at the given index, or NULL if ndx is out
       of bounds or if no PATH_INFO is available.

       e.g. if PATH_INFO=/a/b/c, passing 0 to this function would return
       "a", passing 2 would return "c", and passing anything greater than 2
       would return NULL.
    */
    char const * cson_cgi_path_part_cstr( cson_cgi_cx * cx, unsigned short ndx );

    /**
       Functionally equivalent to cson_cgi_path_part_cstr(), but
       returns the underlying value as a cson value handle. That handle
       is owned by the underlying PATH_INFO_SPLIT array (which is
       owned by the "e" environment object).

       Unless the client has mucked with the PATH_INFO_SPLIT data, the
       returned value will (if it is not NULL) have a logical type of
       String.
    */
    cson_value * cson_cgi_path_part( cson_cgi_cx * cx, unsigned short ndx );
    
    /**
       Sets or unsets a key in the "user" environment/namespace. If v is NULL
       then the value is removed, otherwise it is set/replaced.

       Returns 0 on success. If key is NULL or has a length of 0 then
       cson_rc.ArgError is returned.

       The user namespace object can be fetched via
       cson_cgi_env_get_val('a',...).

       On success ownership of v is transfered to (or shared with) the
       cson_cgi API. On error ownership of v is not modified. Aside from
    */
    int cson_cgi_setenv( cson_cgi_cx * cx, char const * key, cson_value * v );

    /**
       This function is not implemented, but exists as a convenient
       place to document the cson_cgi config file format.
       
       cson_cgi_init() accepts the name of a configuration file
       (assumed to be in JSON format) to read during
       initialization. The library optionally uses the configuration
       to change certain aspects of its behaviour.

       The following commented JSON demonstrates the configuration
       file options:

       @code
       {
       "formatting": { // NOT YET HONORED. Will mimic cson_output_opt.
           "indentation": 1,
           "addNewline": true,
           "addSpaceAfterColon": true,
           "indentSingleMemberValues": true
       },
       "session": { // Options for session handling
           "manager": "file", // name of session manager impl. Should
                              // have a matching entry in "managers" (below)
           "cookieLifetimeMinutes": 10080, // cookie lifetime in minutes
           "cookieName": "cson_session_id", // cookie name for session ID
           "managers": {
               "file": {
                   "sessionDriver": "file", -- cson_cgi-internal session manager name
                   "dir": "./SESSIONS",
                   "prefix": "cson-session-",
                   "suffix": ".json"
               },
               "mysql5": {
                   "sessionDriver": "cpdo", -- cson_cgi-internal session manager name
                   "dsn": "mysql5:dbname=cpdo;host=localhost",
                   "user": "cpdo",
                   "password": "cpdo",
                   "table": "cson_session",
                   "fieldId": "id",
                   "fieldTimestamp": "last_saved",
                   "fieldSession": "json"
               },
               "sqlite3": {
                   "sessionDriver": "cpdo", -- cson_cgi-internal session manager name
                   "dsn": "sqlite3:sessions.sqlite3",
                   "user": null,
                   "password": null,
                   "table": "cson_session",
                   "fieldId": "id",
                   "fieldTimestamp": "last_saved",
                   "fieldSession": "json"
               }
           }
       }
       }
       @endcode

       TODO: allow initialization to take a JSON object, as opposed to
       a filename, so that we can embed the configuration inside client-side
       config data.
    */
    void cson_cgi_config_file();

    /**
       Sets or (if v is NULL) unsets a cookie value.

       v must either be of one of the types (string, integer, double,
       bool, null, NULL) or must be an object with the following
       structure:

       @code
       {
           value: (string, integer, double, bool, or null),
           OPTIONAL path: string,
           OPTIONAL domain: string,
           OPTIONAL expires: integer (Unix epoch timestamp),
           OPTIONAL secure: bool,
           OPTIONAL httponly: bool
       }
       @endcode

       For the object form, if the "value" property is missing or not of
       the correct type then the cookie will not be emitted in the
       HTTP response headers. The other properties are optional. A value
       of NULL or cson_value_null() will cause the expiry field (if set)
       to be ignored. Note, however, that removal will only work
       on the client side if all other cookie parameters match
       (e.g. domain and path).

       Returns 0 on success, non-0 on error.

       A duplicate cookie replaces any previous cookie with the same
       key.
       
       On success ownership of v is shared with the cson_cgi API (via
       reference counting). On error ownership of v is not modified.
    */
    int cson_cgi_cookie_set( cson_cgi_cx * cx, char const * key, cson_value * v );

    /**
       Sets or (if v is NULL) unsets an HTTP cookie value. key may not
       be NULL nor have a length of 0. v must be one of the types
       (string, integer, double, bool, null, NULL). Any other pointer
       arguments may be NULL, in which case they are not used.
       If v is NULL then the JSON null value is used as a placeholder
       value so that when the HTTP headers are generated, the cookie
       can be unset on the client side.

       This function creates an object with the structure documented
       in cson_cgi_cookie_set() and then passes that object to
       cson_cgi_cookie_set(). Any parameters which have NULL/0 values
       are not emitted in the generated object, with the exception of
       (v==NULL), which causes the expiry property to be ignored and a
       value from a time far in the past to be used (so that the
       client will expire it)..

       Returns 0 on success, non-0 on error.

       On success ownership of v is shared with the cson_cgi API (via
       reference counting). On error ownership of v is not modified.
    */
    int cson_cgi_cookie_set2( cson_cgi_cx * cx, char const * key, cson_value * v,
                              char const * domain, char const * path,
                              unsigned int expires, char secure, char httponly );
    
    /**
        Returns the internal "environment" JSON object corresponding
        to the given 'which' letter, which must be one of
        (case-insensitive):

        - g = GET
        - p = POST
        - c = COOKIE
        - e = ENV (i.e. system environment)
        - s = SESSION
        - a = APP (application-specific)

        TODO: s = SESSION

        See cson_cgi_getenv() for more details about each of those.

        Returns NULL if 'which' is not one of the above.

        Note that in the 'e' (system environment) case, making
        modifications to the returned object will NOT also modify the
        system environment.  Likewise, future updates to the system
        environment will not be automatically reflected in the
        returned object.

        The returned object is owned by the cson_cgi environment and
        must not be destroyed by the caller.

        If createIfNeeded is non-0 (true) then the requested
        environment object is created if it was formerly empty. In that
        case, a return value of NULL can indicate an invalid 'which'
        parameter or an allocation error.       

        To get the Object reference to this environment use
        cson_cgi_env_get_obj() or pass the result of this function
        to cson_value_get_object().
 
        The returned value is owned by the cson_cgi API.

        The public API does not provide a way for clients to modify
        several of the internal environment stores, e.g. HTTP GET
        parameters are set only by this framework. However, clients
        can (if needed) get around this by fetching the associated
        "environment object" via this function or
        cson_cgi_env_get_obj(), and modifying it directly. Clients are
        encouraged to use the other public APIs for dealing with the
        environment, however, and are encouraged to not directly modify
        "special" namespaces like the cookie/GET/POST data.        
    */
    cson_value * cson_cgi_env_get_val( cson_cgi_cx * cx, char which, char createIfNeeded );

    /**
       Equivalent to:

       @code
       cson_value_get_object( cson_cgi_env_get_val( which, createIfNeeded ) );
       @endcode

       Note, however, that it is at least theoretically possible that
       cson_cgi_env_get_val() return non-NULL but this function
       returns NULL. If that happens it means that the value returned
       by cson_cgi_env_get_val() is-not-a Object instance, but is
       something else (maybe an array?).
    */
    cson_object * cson_cgi_env_get_obj( cson_cgi_cx * cx, char which, char createIfNeeded );

    /**
       Adds the given key/value to the list of HTTP headers (replacing
       any existing entry with the same name).  If v is NULL then any
       header with the given key is removed from the pending response.

       Returns 0 on success. On success ownership of v is transfered
       to (or shared with) the internal header list. On error,
       ownership of v is not modified.

       If v is not of one of the types (string, integer, double, bool,
       undef, null) then the header will not be output when when
       cson_cgi_response_output_headers() is called. If it is one of
       those types then its stringified value will be its "natural"
       form (for strings and numbers), the integer 0 or 1 for
       booleans, and the number 0 for null.  Note that a literal
       (v==NULL) is treated differently from a JSON null - it UNSETS
       the given header.

       This function should not be used for setting cookies, as they
       require extra url-encoding and possibly additional
       parameters. Use cson_cgi_cookie_set() and
       cson_cgi_cookie_set2() to set cookie headers.
    */
    int cson_cgi_response_header_add( cson_cgi_cx * cx, char const * key, cson_value * v );

    /**
       Returns a cson array value containing the arguments passed
       to cson_cgi_init(). The returned value is owned by the cson_cgi
       API and must not be destroyed by the caller.

       Only returns NULL if initialization of cson_cgi_init() fails
       early on, and is almost certainly indicative of an allocation
       error. If cson_cgi_init() is given a 0-length argument list
       then this function will return an empty array (except in the
       NULL case mentioned above).
    */
    cson_value * cson_cgi_argv(cson_cgi_cx * cx);
    
    /**
       Equivalent to:

       @code
       cson_value_get_array( cson_cgi_argv() );
       @endcode
    */
    cson_array * cson_cgi_argv_array(cson_cgi_cx * cx);

    /**
       Flushes all response headers set via cson_cgi_response_header_add()
       to stdout. The client must output an empty line before the body
       part (if any), and may output custom headers before doing so.

       Do not call this more than once.
    */
    int cson_cgi_response_output_headers(cson_cgi_cx * cx);

    /**
       Outputs the response root object to stdout. If none has been
       set, non-0 is returned.

       Returns 0 on success. On error, partial output might be
       generated.

       Do not call this more than once.
    */
    int cson_cgi_response_output_root(cson_cgi_cx * cx);

    /**
       Outputs the whole response, including headers and the root JSON
       value.

       Returns 0 on success. Fails without side effects if
       no root is set.

       Do not call this more than once.
    */
    int cson_cgi_response_output_all(cson_cgi_cx * cx);
    
    /**
       Don't use this - i need to re-think the JSONP bits.
    
       Returns non-0 (true) if the GET/POST environment contains a key
       named CSON_CGI_KEY_JSONP. If this is the case, then when
       cson_cgi_response_output_headers() is called the Content-type
       is set to "application/javascript".

       If cson_cgi_enable_jsonp() is ever called to set this option
       explicitly, this function does not guess, but uses that value
       instead.

       When JSONP is desired, the generated page output must be
       wrapped in the appropriate JS code.
    */
    char cson_cgi_is_jsonp(cson_cgi_cx * cx);

    /**
       Don't use this - i need to re-think the JSONP bits.

       Sets or unsets JSONP mode. If b is 0 then JSONP guessing is
       explicitly disabled and output is assumed to be JSON. If it is
       non-0 then cson_cgi_guess_content_type() will never return
       "application/javascript".

       When JSONP is desired, the generated page output must be
       wrapped in the appropriate JS code.
    */
    void cson_cgi_enable_jsonp( cson_cgi_cx * cx, char b );

    /**
       Tries to guess the "best" Content-type header value for
       the current session, based on several factors:

       - If the GET/POST data contains a variable named
       CSON_CGI_KEY_JSONP then "application/javascript" is returned.

       - If the HTTP_ACCEPT environment variable is NOT set or
       contains "application/json" then "application/json [possibly
       charset info]" is returned.

       - If the HTTP_ACCEPT environment variable is set but does not
       contain "application/json" then "text/javascript" is returned.

       - If cson_cgi_enable_jsonp() is called and passed a true value,
       "application/javascript" is returned.

       
       If HTTP_ACCEPT_CHARSET is NOT set or contains "utf-8" then
       ";charset=utf-8" is included in the the returned string.
       
       The returned string is static and immutable and suitable for
       use as a Content-type header value. The string is guaranteed to
       be valid until the application exits. Multiple calls to this
       function, with different underlying environment data, can cause
       different results to be returned.

       Returns NULL if it absolutely cannot figure out what to do, but
       currently it has no such logic paths.
    */
    char const * cson_cgi_guess_content_type(cson_cgi_cx * cx);

    /**
       Sets the response content root, replacing any
       existing one (and possibly cleaning it up).

       Returns 0 on success. On success, ownership of v
       is transfered to (or shared with) the cson_cgi
       API. It will be cleaned up at app shutdown time
       or if it is subsequently replaced and has no
       other open references to it.

       On error ownership of v is not modified and any previous
       root is not removed.

       If v is-not-a Object or Array, nor NULL, then cson_rc.TypeError
       is returned. JSON requires either an object or array for the
       root node. Passing NULL will possibly free up any current root
       (depending on its reference count).
    */
    int cson_cgi_response_root_set( cson_cgi_cx * cx, cson_value * v );

    /**
       Fetches the current content root JSON value, as set via
       cson_cgi_response_root_set() or (if no root has been set), as
       defined by createMode, as described below.

       If a content root has been set (or previously initialized)
       then the value of createMode is ignored. If no root has been
       set then this function might try to create one, as described
       here:

       (createMode==0) means not to create a root element if none
       exists already.

       (createMode<0) means to create the root as an Array value.

       (createMode>0) means to create the root as an Object value.

       Returns NULL on allocation error or if no root has been set and
       (createMode==0). On success the returned value is guaranteed to
       be either an Array or Object (see cson_value_get_object() and
       cson_value_get_array()) and it is owned by the cson_cgi API (so
       it must not be destroyed by the caller). If the client needs to
       destroy it, pass NULL to cson_cgi_response_root_set().
    */
    cson_value * cson_cgi_response_root_get( cson_cgi_cx * cx, char createMode );
    
    /**
       Returns the current session ID. If session management is not
       enabled then NULL is returned.

       The returned bytes are owned by the cson_cgi API and are valid
       until the library is cleaned up (via cson_cgi_cleanup_lib() or
       via the normal shutdown process) or the session ID is
       re-generated for some reason. It is best not to hold a
       reference to this, but to copy it if it will be needed later.

       If the return value is not NULL, it is guaranteed to be
       NUL-terminated.
    */
    char const * cson_cgi_session_id(cson_cgi_cx *);

    /**
       Writes a 36-byte (plus one NUL byte) random UUID value to
       dest. dest must be at least 37 bytes long. If dest is NULL this
       function has no side effects.

       This function uses internal RNG state and is not thread-safe.
    */
    void cson_cgi_generate_uuid( cson_cgi_cx * cx, char * dest );

    /**
       Adds v to the API-internal cleanup mechanism. key must be a
       unique key for the given element. Adding another item with that
       key may free the previous one. If freeOnError is true then v is
       passed to cson_value_free() if the key cannot be inserted,
       otherweise ownership of v is not changed on error.

       Returns 0 on success.

       On success, ownership of v is transfered to (or shared with)
       cx, and v will be valid until cx is cleaned up or its key is
       replaced via another call to this function.
    */
    int cson_cgi_gc_add( cson_cgi_cx * cx, char const * key, cson_value * v, char freeOnError );

    /**

    Replaces %XX patterns in str with their equivalent character and
    '+' characters with a single whitespace. %XX patterns which are
    not hexidecimal values are not translated.

    str must be NULL or a NUL-terminated string. If it is NULL or the
    first byte is NUL then 0 is returned and this function has no
    side-effects.
   
    BUGS(?): URL-decoding might have a few bugs/corner cases.

    Returns 0 on success, non-0 on error.
    */
    int cson_cgi_urldecode_inline( char * str );

    /**
       Replaces the current POST data (if any) with the given value.
       Any previous POST data is cleaned up, possibly invalidating
       handles referencing data it holds.

       newPost must be-a Object or Array, or cson_rc.TypeError will be
       returned.
       
       On success 0 is returned and ownership of newPost is transfered
       to cx. On error ownership is not modified.
    */
    int cson_cgi_replace_POST( cson_cgi_cx * cx, cson_value * newPost );

    /**
       Replaces the current GET request data (if any) with the given
       value.  Any previous GET data is cleaned up, possibly
       invalidating handles referencing data it holds.

       On success 0 is returned and ownership of newPost is transfered
       to cx. On error ownership is not modified.
    */
    int cson_cgi_replace_GET( cson_cgi_cx * cx, cson_object * newGet );
    
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

#endif /* WANDERINGHORSE_NET_CSON_CGI_H_INCLUDED */
