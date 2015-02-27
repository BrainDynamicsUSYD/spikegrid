#include "wh/cson/cson_cpdo.h"
#if CSON_ENABLE_CPDO /* we do this here for the sake of the amalgamation build */
#include <string.h> /* strlen() */
#include <assert.h>

#if 0
#include <stdio.h>
#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf
#else
#define MARKER if(0) printf
#endif

#if defined(__cplusplus)
extern "C" {
#endif

cson_value * cson_cpdo_stmt_to_value( cpdo_stmt * st, uint16_t ndx )
{
    if( ! st ) return NULL;
    else
    {
        cpdo_data_type vtype = CPDO_TYPE_ERROR;
        int rc = st->api->get.type( st, ndx, &vtype );
        MARKER("ndx=%u, vtype=%d, rc=%d\n",ndx, vtype,rc);
        if( rc ) return NULL;
        switch( vtype )
        {
          case CPDO_TYPE_NULL:
              return cson_value_null();
          case CPDO_TYPE_INT8:
              return cson_value_new_integer( (cson_int_t)cpdo_get_int8(st,ndx) );
          case CPDO_TYPE_INT16:
              return cson_value_new_integer( (cson_int_t)cpdo_get_int16(st,ndx) );
          case CPDO_TYPE_INT32:
              return cson_value_new_integer( (cson_int_t)cpdo_get_int32(st,ndx) );
          case CPDO_TYPE_INT64:
              return cson_value_new_integer( (cson_int_t)cpdo_get_int64(st,ndx) );
          case CPDO_TYPE_DOUBLE:
              return cson_value_new_double( cpdo_get_double(st, ndx) );
          case CPDO_TYPE_BLOB: {
              /* we'll just hope it's really a string. MySQL reports
                 TEXT fields as type BLOB.
              */
              uint32_t slen = 0;
              void const * bl = cpdo_get_blob( st, ndx, &slen );
              return cson_value_new_string(bl, bl ? slen : 0);
          }
          case CPDO_TYPE_CUSTOM /* we do this for MySQL time/date types */:
          case CPDO_TYPE_STRING: {
              uint32_t slen = 0;
              char const * str = cpdo_get_string( st, ndx, &slen );
              return cson_value_new_string(str, str ? slen : 0);
          }
          default:
              return NULL;
        }
    }
}

cson_value * cson_cpdo_row_to_object2( cpdo_stmt * st, cson_array * colNames )
{
    uint16_t colCount;
    cson_value * objV;
    cson_object * obj;
    cson_string * colName;
    uint16_t i;
    int rc;
    if( ! st ) return NULL;
    colCount = st->api->get.column_count( st );
    if( !colCount || (colCount>cson_array_length_get(colNames)) ) {
        return NULL;
    }
    objV = cson_value_new_object();
    if( ! objV ) return NULL;
    obj = cson_value_get_object( objV );
    for( i = 0; i < colCount; ++i )
    {
        cson_value * v = NULL;
        colName = cson_value_get_string( cson_array_get( colNames, i ) );
        if( ! colName ) goto error;
        v = cson_cpdo_stmt_to_value( st, i );
        if( ! v ) goto error;
        rc = cson_object_set_s( obj, colName, v );
        if( rc )
        {
            cson_value_free(v);
            goto error;
        }
    }
    return objV;
    error:
    cson_value_free( objV );
    return NULL;
}

cson_value * cson_cpdo_row_to_object( cpdo_stmt * st )
{
    uint16_t colCount;
    cson_value * objV;
    cson_object * obj;
    char const * colName;
    uint16_t i;
    int rc;
    if( ! st ) return NULL;
    colCount = st->api->get.column_count( st );
    if( ! colCount ) return NULL;
    objV = cson_value_new_object();
    if( ! objV ) return NULL;
    obj = cson_value_get_object( objV );
    for( i = 0; i < colCount; ++i )
    {
        cson_value * v = NULL;
        colName = st->api->get.column_name( st, i );
        if( ! colName ) goto error;
        v = cson_cpdo_stmt_to_value( st, i );
        if( ! v ) goto error;
        rc = cson_object_set( obj, colName, v );
        if( rc )
        {
            cson_value_free(v);
            goto error;
        }
    }
    return objV;
    error:
    cson_value_free( objV );
    return NULL;
}

cson_value * cson_cpdo_row_to_array( cpdo_stmt * st )
{
    uint16_t colCount;
    cson_value * arV;
    cson_array * ar;
    uint16_t i;
    int rc;
    if( ! st ) return NULL;
    colCount = st->api->get.column_count( st );
    if( ! colCount ) return NULL;
    arV = cson_value_new_array();
    if( ! arV ) return NULL;
    ar = cson_value_get_array( arV );
    for( i = 0; i < colCount; ++i )
    {
        cson_value * v = NULL;
        v = cson_cpdo_stmt_to_value( st, i );
        if( ! v ) goto error;
        rc = cson_array_append( ar, v );
        if( rc )
        {
            cson_value_free(v);
            goto error;
        }
    }
    return arV;
    error:
    cson_value_free( arV );
    return NULL;
}

cson_value * cson_cpdo_stmt_col_names( cpdo_stmt * st )
{
    cson_value * aryV = NULL;
    cson_array * ary = NULL;
    char const * colName = NULL;
    uint16_t i = 0;
    int rc = 0;
    uint16_t colCount = 0;
    cson_value * newVal = NULL;
    assert(st);
    colCount = st->api->get.column_count( st );
    if( ! colCount ) return NULL;
    aryV = cson_value_new_array();
    if( ! aryV ) return NULL;
    ary = cson_value_get_array(aryV);
    assert(ary);
    for( i = 0; (0==rc) && (i < colCount); ++i )
    {
        colName = st->api->get.column_name(st, i);
        if( ! colName ) rc = cson_rc.AllocError;
        else
        {
            newVal = cson_value_new_string(colName, strlen(colName));
            if( NULL == newVal )
            {
                rc = cson_rc.AllocError;
            }
            else
            {
                rc = cson_array_set( ary, i, newVal );
                if( rc ) cson_value_free( newVal );
            }
        }
    }
    if( 0 == rc ) return aryV;
    else
    {
        cson_value_free(aryV);
        return NULL;
    }
}

/**
    Internal impl of cson_sqlite3_stmt_to_json() when the 'fat'
    parameter is non-0.
*/
static int cson_cpdo_stmt_to_json_fat( cpdo_stmt * st, cson_value ** tgt )
{
#define RETURN(RC) { if(rootV) cson_value_free(rootV); return RC; } (void)0
    if( ! tgt || !st ) return cson_rc.ArgError;
    else
    {
        cson_value * rootV = NULL;
        cson_object * root = NULL;
        cson_value * colsV = NULL;
        cson_array * cols = NULL;
        cson_value * rowsV = NULL;
        cson_array * rows = NULL;
        cson_value * row = NULL;
        int rc = 0;
        uint16_t colCount = st->api->get.column_count(st);
        if( ! colCount ) return cson_rc.ArgError;
        rootV = cson_value_new_object();
        if( ! rootV ) return cson_rc.AllocError;
        colsV = cson_cpdo_stmt_col_names(st);
        if( ! colsV ) RETURN(cson_rc.AllocError);
        cols = cson_value_get_array(colsV);
        root = cson_value_get_object(rootV);
        rc = cson_object_set( root, "columns", colsV );
        if( rc )
        {
            cson_value_free(colsV);
            RETURN(rc);
        }
        colsV = NULL;
        
        rowsV = cson_value_new_array();
        if( ! rowsV ) RETURN(cson_rc.AllocError);
        rc = cson_object_set( root, "rows", rowsV );
        if( 0 != rc )
        {
            cson_value_free( rowsV );
            RETURN(rc);
        }
        rows = cson_value_get_array(rowsV);
        assert(rows);
        while( CPDO_STEP_OK == st->api->step(st) )
        {
            row = cson_cpdo_row_to_object2( st, cols );
            if( ! row ) RETURN(cson_rc.UnknownError);
            rc = cson_array_append( rows, row );
            if( 0 != rc )
            {
                cson_value_free( row );
                RETURN(rc);
            }
        }
        *tgt = rootV;
        return 0;
    }
#undef RETURN
}

/**
    Internal impl of cson_sqlite3_stmt_to_json() when the 'fat'
    parameter is 0.
*/
static int cson_cpdo_stmt_to_json_slim( cpdo_stmt * st, cson_value ** tgt )
{
#define RETURN(RC) { if(rootV) cson_value_free(rootV); return RC; }
    if( ! tgt || !st ) return cson_rc.ArgError;
    else
    {
        cson_value * rootV = NULL;
        cson_object * root = NULL;
        cson_value * aryV = NULL;
        cson_value * rowsV = NULL;
        cson_array * rows = NULL;
        int rc = 0;
        uint16_t const colCount = st->api->get.column_count(st);
        if( !colCount ) return cson_rc.ArgError;
        rootV = cson_value_new_object();
        if( ! rootV ) return cson_rc.AllocError;
        aryV = cson_cpdo_stmt_col_names(st);
        if( ! aryV ) RETURN(cson_rc.AllocError);
        root = cson_value_get_object(rootV);
        rc = cson_object_set( root, "columns", aryV );
        if( rc )
        {
            cson_value_free(aryV);
            RETURN(rc);
        }
        aryV = NULL;
        rowsV = cson_value_new_array();
        if( ! rowsV ) RETURN(cson_rc.AllocError);
        rc = cson_object_set( root, "rows", rowsV );
        if( 0 != rc )
        {
            cson_value_free( rowsV );
            RETURN(rc);
        }
        rows = cson_value_get_array(rowsV);
        assert(rows);
        while( CPDO_STEP_OK == st->api->step(st) )
        {
            aryV = cson_cpdo_row_to_array(st);
            if( ! aryV ) RETURN(cson_rc.UnknownError);
            rc = cson_array_append( rows, aryV );
            if( 0 != rc )
            {
                cson_value_free( aryV );
                RETURN(rc);
            }
        }
        *tgt = rootV;
        return 0;
    }
#undef RETURN
}

int cson_cpdo_stmt_to_json( cpdo_stmt * st, cson_value ** tgt, char fat )
{
    return fat
        ? cson_cpdo_stmt_to_json_fat(st,tgt)
        : cson_cpdo_stmt_to_json_slim(st,tgt)
        ;
}

int cson_cpdo_sql_to_json( cpdo_driver * db, cson_value ** tgt, char const * sql, char fat )
{
    if( !db || !tgt || !sql || !*sql ) return cson_rc.ArgError;
    else
    {
        cpdo_stmt * st = NULL;
        int rc = cpdo_prepare( db, &st, sql, strlen(sql) );
        if( 0 != rc ) return cson_rc.IOError /* FIXME: Better error code? */;
        rc = cson_cpdo_stmt_to_json( st, tgt, fat );
        st->api->finalize( st );
        return rc;
    }        
}

int cson_cpdo_parse_json( cpdo_stmt * st, uint16_t ndx, cson_value ** tgt )
{
    if( ! st || ! tgt) return cson_rc.ArgError;
    else
    {
        void const * blob = NULL;
        uint32_t slen = 0;
        int rc = st->api->get.blob( st, ndx, &blob, &slen );
        if( rc || ! slen)
        {
            char const * str = NULL;
            rc = st->api->get.string( st, ndx, &str, &slen );
            if( rc ) return cson_rc.TypeError;
            else if( !slen ) return 0;
            blob = str;
        }
        return cson_parse_string( tgt, (char const *)blob, slen, NULL, NULL );
    }
}

int cson_cpdo_bind_json( cpdo_stmt * st, uint16_t ndx, cson_value const * jv )
{
    if( ! st ) return cpdo_rc.ArgError;
    else if( ! jv || (!cson_value_is_array(jv) && !cson_value_is_object(jv)) )
    {
        return st->api->bind.null( st, ndx );
    }
    else
    {
        int rc = 0;
        cson_buffer buf = cson_buffer_empty;
        rc = cson_output_buffer( jv, &buf, NULL );
        if( rc ) return rc;
        rc = st->api->bind.blob( st, ndx, buf.mem, buf.used );
        if( rc )
        { /* try again as string ... */
             rc = st->api->bind.string( st, ndx, (char const *)buf.mem, buf.used );
        }
        cson_buffer_reserve( &buf, 0 );
        return rc;
    }
}    

int cson_cpdo_bind_value( cpdo_stmt * st, uint16_t ndx, cson_value const * v )
{
    if(!st || !ndx) return cpdo_rc.ArgError;
    else if( !v || cson_value_is_null(v) ){
        return st->api->bind.null(st, ndx);
    }
    else if( cson_value_is_integer(v) || cson_value_is_bool(v) ) {
        return st->api->bind.i64(st, ndx, cson_value_get_integer(v));
    }
    else if( cson_value_is_double(v) ) {
        return st->api->bind.dbl(st, ndx, cson_value_get_double(v));
    }
    else if( cson_value_is_string(v) ) {
        char const * str = cson_value_get_cstr(v);
        unsigned int slen = cson_string_length_bytes(cson_value_get_string(v));
        int rc = st->api->bind.string(st, ndx, str, slen);
        if(rc){ /* try again as blob... */
            rc = st->api->bind.blob(st, ndx, str, slen);
        }
        return rc;
    }
    else {
        return cpdo_rc.TypeError;
    }
}



#undef MARKER
#if defined(__cplusplus)
} /*extern "C"*/
#endif

#endif/*CSON_ENABLE_CPDO*/
