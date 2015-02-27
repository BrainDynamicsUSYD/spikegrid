/** @file cson_sqlite3.h

This file contains cson's public sqlite3-to-JSON API declarations
and API documentation. If CSON_ENABLE_SQLITE3 is not defined,
or is defined to 0, then including this file will have no side-effects
other than defining CSON_ENABLE_SQLITE3 (if it was not defined) to 0
and defining a few include guard macros. i.e. if CSON_ENABLE_SQLITE3
is not set to a true value then the API is not visible.

This API requires that <sqlite3.h> be in the INCLUDES path and that
the client eventually link to (or directly embed) the sqlite3 library.
*/
#if !defined(WANDERINGHORSE_NET_CSON_SQLITE3_H_INCLUDED)
#define WANDERINGHORSE_NET_CSON_SQLITE3_H_INCLUDED 1
#if !defined(CSON_ENABLE_SQLITE3)
#  if defined(DOXYGEN)
#    define CSON_ENABLE_SQLITE3 1
#  else
#    define CSON_ENABLE_SQLITE3 0
#  endif
#endif

#include "cson.h"
#if CSON_ENABLE_SQLITE3 /* we do this here for the sake of the amalgamation build */
#include <sqlite3.h>

#if defined(__cplusplus)
extern "C" {
#endif

/**
   Converts a single value from a single 0-based column index to its JSON
   equivalent.

   On success it returns a new JSON value, which will have a different concrete
   type depending on the field type reported by sqlite3_column_type(st,col):

   Integer, double, null, or string (TEXT and BLOB data, though not
   all blob data is legal for a JSON string).

   st must be a sqlite3_step()'d row and col must be a 0-based column
   index within that result row.
 */       
cson_value * cson_sqlite3_column_to_value( sqlite3_stmt * st, int col );

/**
   Creates a JSON Array object containing the names of all columns
   of the given prepared statement handle. 
    
   Returns a new array value on success, which the caller owns. Its elements
   are in the same order as in the underlying query.

   On error NULL is returned.
    
   st is not traversed or freed by this function - only the column
   count and names are read.
*/
cson_value * cson_sqlite3_column_names( sqlite3_stmt * st );

/**
   Creates a JSON Object containing key/value pairs corresponding
   to the result columns in the current row of the given statement
   handle. st must be a sqlite3_step()'d row result.

   On success a new Object is returned which is owned by the
   caller. On error NULL is returned.

   cson_sqlite3_column_to_value() is used to convert each column to a
   JSON value, and the column names are taken from
   sqlite3_column_name().
*/
cson_value * cson_sqlite3_row_to_object( sqlite3_stmt * st );
/**
   Functionally almost identical to cson_sqlite3_row_to_object(), the
   only difference being how the result objects gets its column names.
   st must be a freshly-step()'d handle holding a result row.
   colNames must be an Array with at least the same number of columns
   as st. If it has fewer, NULL is returned and this function has
   no side-effects.

   For each column in the result set, the colNames entry at the same
   index is used for the column key. If a given entry is-not-a String
   then conversion will fail and NULL will be returned.

   The one reason to prefer this over cson_sqlite3_row_to_object() is
   that this one can share the keys across multiple rows (or even
   other JSON containers), whereas the former makes fresh copies of
   the column names for each row.

*/
cson_value * cson_sqlite3_row_to_object2( sqlite3_stmt * st,
                                          cson_array * colNames );

/**
   Similar to cson_sqlite3_row_to_object(), but creates an Array
   value which contains the JSON-form values of the given result
   set row.
*/
cson_value * cson_sqlite3_row_to_array( sqlite3_stmt * st );
/**
    Converts the results of an sqlite3 SELECT statement to JSON,
    in the form of a cson_value object tree.
    
    st must be a prepared, but not yet traversed, SELECT query.
    tgt must be a pointer to NULL (see the example below). If
    either of those arguments are NULL, cson_rc.ArgError is returned.
    
    This walks the query results and returns a JSON object which
    has a different structure depending on the value of the 'fat'
    argument.
    
    
    If 'fat' is 0 then the structure is:
    
    @code
    {
        "columns":["colName1",..."colNameN"],
        "rows":[
            [colVal0, ... colValN],
            [colVal0, ... colValN],
            ...
        ]
    }
    @endcode
    
    In the "non-fat" format the order of the columns and row values is
    guaranteed to be the same as that of the underlying query.
    
    If 'fat' is not 0 then the structure is:
    
    @code
    {
        "columns":["colName1",..."colNameN"],
        "rows":[
            {"colName1":value1,..."colNameN":valueN},
            {"colName1":value1,..."colNameN":valueN},
            ...
        ]
    }
    @endcode

    In the "fat" format, the order of the "columns" entries is guaranteed
    to be the same as the underlying query fields, but the order
    of the keys in the "rows" might be different and might in fact
    change when passed through different JSON implementations,
    depending on how they implement object key/value pairs.

    On success it returns 0 and assigns *tgt to a newly-allocated
    JSON object tree (using the above structure), which the caller owns.
    If the query returns no rows, the "rows" value will be an empty
    array, as opposed to null.
    
    On error non-0 is returned and *tgt is not modified.
    
    The error code cson_rc.IOError is used to indicate a db-level
    error, and cson_rc.TypeError is returned if sqlite3_column_count(st)
    returns 0 or less (indicating an invalid or non-SELECT statement).
    
    The JSON data types are determined by the column type as reported
    by sqlite3_column_type():
    
    SQLITE_INTEGER: integer
    
    SQLITE_FLOAT: double
    
    SQLITE_TEXT or SQLITE_BLOB: string, and this will only work if
    the data is UTF8 compatible.
    
    If the db returns a literal or SQL NULL for a value it is converted
    to a JSON null. If it somehow finds a column type it cannot handle,
    the value is also converted to a NULL in the output.

    Example
    
    @code
    cson_value * json = NULL;
    int rc = cson_sqlite3_stmt_to_json( myStatement, &json, 1 );
    if( 0 != rc ) { ... error ... }
    else {
        cson_output_FILE( json, stdout, NULL );
        cson_value_free( json );
    }
    @endcode
*/
int cson_sqlite3_stmt_to_json( sqlite3_stmt * st, cson_value ** tgt, char fat );

/**
    A convenience wrapper around cson_sqlite3_stmt_to_json(), which
    takes SQL instead of a sqlite3_stmt object. It has the same
    return value and argument semantics as that function.
*/
int cson_sqlite3_sql_to_json( sqlite3 * db, cson_value ** tgt, char const * sql, char fat );

/**
   Binds a JSON value to a 1-based parameter index in a prepared SQL
   statement. v must be NULL or one of one of the types (null, string,
   integer, double, boolean, array). Booleans are bound as integer 0
   or 1. NULL or null are bound as SQL NULL. Integers are bound as
   64-bit ints. Strings are bound using sqlite3_bind_text() (as
   opposed to text16), but we could/should arguably bind them as
   blobs.

   If v is an Array then ndx is is used as a starting position
   (1-based) and each item in the array is bound to the next parameter
   position (starting and ndx, though the array uses 0-based offsets).

   TODO: add Object support for named parameters.

   Returns 0 on success, non-0 on error.
 */
int cson_sqlite3_bind_value( sqlite3_stmt * st, int ndx, cson_value const * v );
    
#if defined(__cplusplus)
} /*extern "C"*/
#endif
    
#endif /* CSON_ENABLE_SQLITE3 */
#endif /* WANDERINGHORSE_NET_CSON_SQLITE3_H_INCLUDED */
