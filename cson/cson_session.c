#include "wh/cson/cson_session.h"
#include <string.h> /* strlen() */
#include <stdlib.h> /* memcpy() */
#include <assert.h>


/**
   Maximum name length for cson_sessmgr_register(),
   including the trailing NUL.
*/
enum {
   CsonSessionNameLen = 32
};

typedef struct cson_sessmgr_reg cson_sessmgr_reg;
/**
   Holds name-to-factory mappings for cson_sessmgr implementations.
*/
struct cson_sessmgr_reg
{
    char name[CsonSessionNameLen];
    cson_sessmgr_factory_f factory;
};


#if !CSON_ENABLE_CPDO
int cson_sessmgr_cpdo( cson_sessmgr ** tgt, cson_object const * opt )
{
    return cson_rc.UnsupportedError;
}
#endif
#if !CSON_ENABLE_WHIO
int cson_sessmgr_whio_ht( cson_sessmgr ** tgt, cson_object const * opt )
{
    return cson_rc.UnsupportedError;
}
int cson_sessmgr_whio_epfs( cson_sessmgr ** tgt, cson_object const * opt )
{
    return cson_rc.UnsupportedError;
}
#endif

/**
   Holds the list of registered cson_sessmgr implementations. Used by
   cson_sessmgr_register(), cson_sessmgr_load(), and
   cson_sessmgr_names().

   Maintenance reminder: the API docs promise that at least 10 slots
   are initially available.
*/
static cson_sessmgr_reg CsonSessionReg[] = {
{{'f','i','l','e',0},cson_sessmgr_file},
#if CSON_ENABLE_CPDO
{{'c','p','d','o',0},cson_sessmgr_cpdo},
#endif
#if CSON_ENABLE_WHIO
{{'w','h','i','o','_','h','t',0},cson_sessmgr_whio_ht},
{{'w','h','i','o','_','e','p','f','s',0},cson_sessmgr_whio_epfs},
#endif
#define REG {{0},NULL}
REG,REG,REG,REG,REG,
REG,REG,REG,REG,REG
#undef REG
};
static const unsigned int CsonSessionRegLen = sizeof(CsonSessionReg)/sizeof(CsonSessionReg[0]);


int cson_sessmgr_register( char const * name, cson_sessmgr_factory_f f )
{
    if( ! name || !*name || !f ) return cson_rc.ArgError;
    else
    {
        cson_sessmgr_reg * r = CsonSessionReg;
        unsigned int nlen = strlen(name);
        unsigned int i = 0;
        if( nlen >= CsonSessionNameLen ) return cson_rc.RangeError;
        for( ; i < CsonSessionRegLen; ++i, ++r )
        {
            if( r->name[0] ) continue;
            memcpy( r->name, name, nlen );
            r->name[nlen] = 0;
            r->factory = f;
            return 0;
        }
        return cson_rc.RangeError;
    }
}


int cson_sessmgr_load( char const * name, cson_sessmgr ** tgt, cson_object const * opt )
{
    if( ! name || !*name || !tgt ) return cson_rc.ArgError;
    else
    {
        cson_sessmgr_reg const * r = CsonSessionReg;
        unsigned int i = 0;
        for( ; i < CsonSessionRegLen; ++i, ++r )
        {
            if( ! r->name[0] ) break /* end of list */;
            else if( 0 != strcmp( r->name, name ) ) continue;
            else
            {
                assert( NULL != r->factory );
                return r->factory( tgt, opt );
            }
            
        }
        return cson_rc.UnsupportedError;
    }
}

char const * const * cson_sessmgr_names()
{
    static char const * names[sizeof(CsonSessionReg)/sizeof(CsonSessionReg[0])+1];
    unsigned int i = 0;
    cson_sessmgr_reg const * r = CsonSessionReg;
    for( ; i < CsonSessionRegLen; ++i, ++r )
    {
        /*
          pedantic threading note: as long as this function is not
          used concurrently with cson_sessmgr_register(), the worst we
          will do here if this function is called, or its results
          used, concurrently is overwrite in-use values with the same
          values.
         */
        names[i] = r->name[0] ? r->name : NULL;
    }
    names[i] = NULL;
    return names;
}
