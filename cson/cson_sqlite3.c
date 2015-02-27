/** @file cson_sqlite3.c

This file contains the implementation code for the cson
sqlite3-to-JSON API.

License: the same as the cson core library.

Author: Stephan Beal (http://wanderinghorse.net/home/stephan)
*/
#include "wh/cson/cson_sqlite3.h"
#if CSON_ENABLE_SQLITE3 /* we do this here for the sake of the amalgamation build */
#include <assert.h>
#include <string.h> /* strlen() */

#if 0
#include <stdio.h>
#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf
#else
#define MARKER if(0) printf
#endif

#if defined(__cplusplus)
extern "C" {
#endif

cson_value * cson_sqlite3_column_to_value( sqlite3_stmt * st, int col )
{
    if( ! st ) return NULL;
    else
    {
#if 0
        sqlite3_value * val = sqlite3_column_type(st,col);
        int const vtype = val ? sqlite3_value_type(val) : -1;
        if( ! val ) return cson_value_null();
#else
        int const vtype = sqlite3_column_type(st,col);
#endif
        switch( vtype )
        {
          case SQLITE_NULL:
              return cson_value_null();
          case SQLITE_INTEGER:
              /* FIXME: for large integers fall back to Double instead. */
              return cson_value_new_integer( (cson_int_t) sqlite3_column_int64(st, col)  );
          case SQLITE_FLOAT:
              return cson_value_new_double( sqlite3_column_double(st, col) );
          case SQLITE_BLOB: /* arguably fall through... */
          case SQLITE_TEXT: {
              char const * str = (char const *)sqlite3_column_text(st,col);
              return cson_value_new_string(str, str ? strlen(str) : 0);
          }
          default:
              return NULL;
        }
    }
}

cson_value * cson_sqlite3_column_names( sqlite3_stmt * st )
{
    cson_value * aryV = NULL;
    cson_array * ary = NULL;
    char const * colName = NULL;
    int i = 0;
    int rc = 0;
    int colCount = 0;
    assert(st);
    colCount = sqlite3_column_count(st);
    if( colCount <= 0 ) return NULL;
    
    aryV = cson_value_new_array();
    if( ! aryV ) return NULL;
    ary = cson_value_get_array(aryV);
    assert(ary);
    for( i = 0; (0 ==rc) && (i < colCount); ++i )
    {
        colName = sqlite3_column_name( st, i );
        if( ! colName ) rc = cson_rc.AllocError;
        else
        {
            rc = cson_array_set( ary, (unsigned int)i,
                    cson_value_new_string(colName, strlen(colName)) );
        }
    }
    if( 0 == rc ) return aryV;
    else
    {
        cson_value_free(aryV);
        return NULL;
    }
}


cson_value * cson_sqlite3_row_to_object2( sqlite3_stmt * st,
                                          cson_array * colNames )
{
    cson_value * rootV = NULL;
    cson_object * root = NULL;
    cson_string * colName = NULL;
    int i = 0;
    int rc = 0;
    cson_value * currentValue = NULL;
    int const colCount = sqlite3_column_count(st);
    if( !colCount || (colCount>cson_array_length_get(colNames)) ) {
        return NULL;
    }
    rootV = cson_value_new_object();
    if(!rootV) return NULL;
    root = cson_value_get_object(rootV);
    for( i = 0; i < colCount; ++i )
    {
        colName = cson_value_get_string( cson_array_get( colNames, i ) );
        if( ! colName ) goto error;
        currentValue = cson_sqlite3_column_to_value(st,i);
        if( ! currentValue ) currentValue = cson_value_null();
        rc = cson_object_set_s( root, colName, currentValue );
        if( 0 != rc )
        {
            cson_value_free( currentValue );
            goto error;
        }
    }
    goto end;
    error:
    cson_value_free( rootV );
    rootV = NULL;
    end:
    return rootV;
}


cson_value * cson_sqlite3_row_to_object( sqlite3_stmt * st )
{
#if 0
    cson_value * arV = cson_sqlite3_column_names(st);
    cson_array * ar = NULL;
    cson_value * rc = NULL;
    if(!arV) return NULL;
    ar = cson_value_get_array(arV);
    assert( NULL != ar );
    rc = cson_sqlite3_row_to_object2(st, ar);
    cson_value_free(arV);
    return rc;
#else
    cson_value * rootV = NULL;
    cson_object * root = NULL;
    char const * colName = NULL;
    int i = 0;
    int rc = 0;
    cson_value * currentValue = NULL;
    int const colCount = sqlite3_column_count(st);
    if( !colCount ) return NULL;
    rootV = cson_value_new_object();
    if(!rootV) return NULL;
    root = cson_value_get_object(rootV);
    for( i = 0; i < colCount; ++i )
    {
        colName = sqlite3_column_name( st, i );
        if( ! colName ) goto error;
        currentValue = cson_sqlite3_column_to_value(st,i);
        if( ! currentValue ) currentValue = cson_value_null();
        rc = cson_object_set( root, colName, currentValue );
        if( 0 != rc )
        {
            cson_value_free( currentValue );
            goto error;
        }
    }
    goto end;
    error:
    cson_value_free( rootV );
    rootV = NULL;
    end:
    return rootV;
#endif
}

cson_value * cson_sqlite3_row_to_array( sqlite3_stmt * st )
{
    cson_value * aryV = NULL;
    cson_array * ary = NULL;
    int i = 0;
    int rc = 0;
    int const colCount = sqlite3_column_count(st);
    if( ! colCount ) return NULL;
    aryV = cson_value_new_array();
    if( ! aryV ) return NULL;
    ary = cson_value_get_array(aryV);
    rc = cson_array_reserve(ary, (unsigned int) colCount );
    if( 0 != rc ) goto error;

    for( i = 0; i < colCount; ++i ){
        cson_value * elem = cson_sqlite3_column_to_value(st,i);
        if( ! elem ) goto error;
        rc = cson_array_append(ary,elem);
        if(0!=rc)
        {
            cson_value_free( elem );
            goto end;
        }
    }
    goto end;
    error:
    cson_value_free(aryV);
    aryV = NULL;
    end:
    return aryV;
}

    
/**
    Internal impl of cson_sqlite3_stmt_to_json() when the 'fat'
    parameter is non-0.
*/
static int cson_sqlite3_stmt_to_json_fat( sqlite3_stmt * st, cson_value ** tgt )
{
#define RETURN(RC) { if(rootV) cson_value_free(rootV); return RC; }
    if( ! tgt || !st ) return cson_rc.ArgError;
    else
    {
        cson_value * rootV = NULL;
        cson_object * root = NULL;
        cson_value * colsV = NULL;
        cson_array * cols = NULL;
        cson_value * rowsV = NULL;
        cson_array * rows = NULL;
        cson_value * objV = NULL;
        int rc = 0;
        int const colCount = sqlite3_column_count(st);
        if( colCount <= 0 ) return cson_rc.ArgError;
        rootV = cson_value_new_object();
        if( ! rootV ) return cson_rc.AllocError;
        colsV = cson_sqlite3_column_names(st);
        if( ! colsV )
        {
            cson_value_free( rootV );
            RETURN(cson_rc.AllocError);
        }
        cols = cson_value_get_array(colsV);
        assert(NULL != cols);
        root = cson_value_get_object(rootV);
        rc = cson_object_set( root, "columns", colsV );
        if( rc )
        {
            cson_value_free( colsV );
            RETURN(rc);
        }
        rowsV = cson_value_new_array();
        if( ! rowsV ) RETURN(cson_rc.AllocError);
        rc = cson_object_set( root, "rows", rowsV );
        if( rc )
        {
            cson_value_free( rowsV );
            RETURN(rc);
        }
        rows = cson_value_get_array(rowsV);
        assert(rows);
        while( SQLITE_ROW == sqlite3_step(st) )
        {
            objV = cson_sqlite3_row_to_object2(st, cols);
            if( ! objV ) RETURN(cson_rc.UnknownError);
            rc = cson_array_append( rows, objV );
            if( rc )
            {
                cson_value_free( objV );
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
static int cson_sqlite3_stmt_to_json_slim( sqlite3_stmt * st, cson_value ** tgt )
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
        int const colCount = sqlite3_column_count(st);
        if( colCount <= 0 ) return cson_rc.ArgError;
        rootV = cson_value_new_object();
        if( ! rootV ) return cson_rc.AllocError;
        aryV = cson_sqlite3_column_names(st);
        if( ! aryV )
        {
            cson_value_free( rootV );
            RETURN(cson_rc.AllocError);
        }
        root = cson_value_get_object(rootV);
        rc = cson_object_set( root, "columns", aryV );
        if( rc )
        {
            cson_value_free( aryV );
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
        while( SQLITE_ROW == sqlite3_step(st) )
        {
            aryV = cson_sqlite3_row_to_array(st);
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

int cson_sqlite3_stmt_to_json( sqlite3_stmt * st, cson_value ** tgt, char fat )
{
    return fat
        ? cson_sqlite3_stmt_to_json_fat(st,tgt)
        : cson_sqlite3_stmt_to_json_slim(st,tgt)
        ;
}

int cson_sqlite3_sql_to_json( sqlite3 * db, cson_value ** tgt, char const * sql, char fat )
{
    if( !db || !tgt || !sql || !*sql ) return cson_rc.ArgError;
    else
    {
        sqlite3_stmt * st = NULL;
        int rc = sqlite3_prepare_v2( db, sql, -1, &st, NULL );
        if( 0 != rc ) return cson_rc.IOError /* FIXME: Better error code? */;
        rc = cson_sqlite3_stmt_to_json( st, tgt, fat );
        sqlite3_finalize( st );
        return rc;
    }        
}

int cson_sqlite3_bind_value( sqlite3_stmt * st, int ndx, cson_value const * v )
{
    int rc = 0;
    char convertErr = 0;
    if(!st) return cson_rc.ArgError;
    else if( ndx < 1 ) {
        rc = cson_rc.RangeError;
    }
    else if( cson_value_is_array(v) ){
        cson_array * ar = cson_value_get_array(v);
        unsigned int len = cson_array_length_get(ar);
        unsigned int i;
        assert(NULL != ar);
        for( i = 0; !rc && (i < len); ++i ){
            rc = cson_sqlite3_bind_value( st, (int)i+ndx,
                                          cson_array_get(ar, i));
        }
    }
    else if(!v || cson_value_is_null(v)){
        rc = sqlite3_bind_null(st,ndx);
        convertErr = 1;
    }
    else if( cson_value_is_double(v) ){
        rc = sqlite3_bind_double( st, ndx, cson_value_get_double(v) );
        convertErr = 1;
    }
    else if( cson_value_is_bool(v) ){
        rc = sqlite3_bind_int( st, ndx, cson_value_get_bool(v) ? 1 : 0 );
        convertErr = 1;
    }
    else if( cson_value_is_integer(v) ){
        rc = sqlite3_bind_int64( st, ndx, cson_value_get_integer(v) );
        convertErr = 1;
    }
    else if( cson_value_is_string(v) ){
        cson_string const * s = cson_value_get_string(v);
        rc = sqlite3_bind_text( st, ndx,
                                cson_string_cstr(s),
                                cson_string_length_bytes(s),
                                SQLITE_TRANSIENT);
        convertErr = 1;
    }
    else {
        rc = cson_rc.TypeError;
    }
    if(convertErr && rc) switch(rc){
      case SQLITE_TOOBIG:
      case SQLITE_RANGE: rc = cson_rc.RangeError; break;
      case SQLITE_NOMEM: rc = cson_rc.AllocError; break;
      case SQLITE_IOERR: rc = cson_rc.IOError; break;
      default: rc = cson_rc.UnknownError; break;
    };
    return rc;
}


#if defined(__cplusplus)
} /*extern "C"*/
#endif
#undef MARKER
#endif /* CSON_ENABLE_SQLITE3 */
