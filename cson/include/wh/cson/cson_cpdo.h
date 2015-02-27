/** @file cson_cpdo.h

This file contains cson's public cpdo-to-JSON API declarations
and API documentation. If CSON_ENABLE_CPDO is not defined,
or is defined to 0, then including this file will have no side-effects
other than defining CSON_ENABLE_CPDO (if it was not defined) to 0
and defining a few include guard macros. i.e. if CSON_ENABLE_CPDO
is not set to a true value then the API is not visible.

This API requires that <wh/cpdo/cpdo_amalgamation.h> be in the
INCLUDES path and that the client eventually link to (or directly
embed) the cpdo library.
*/
#if !defined(WANDERINGHORSE_NET_CSON_CPDO_H_INCLUDED)
#define WANDERINGHORSE_NET_CSON_CPDO_H_INCLUDED 1
#if !defined(CSON_ENABLE_CPDO)
#  if defined(DOXYGEN)
#    define CSON_ENABLE_CPDO 1
#  else
#    define CSON_ENABLE_CPDO 0
#  endif
#endif

#include "cson.h"
#if CSON_ENABLE_CPDO /* we do this here for the sake of the amalgamation build */
#include "wh/cpdo/cpdo_amalgamation.h"

#if defined(__cplusplus)
extern "C" {
#endif

/**
    Converts the results of an cpdo SELECT statement to JSON,
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
    depending on how they implement object key/value pairs. For most
    data sets with small values, the "thin" format produces much
    less output than the fat format.

    On success it returns 0 and assigns *tgt to a newly-allocated
    JSON object tree (using the above structure), which the caller owns.
    If the query returns no rows, the "rows" value will be an empty
    array, as opposed to null.
    
    On error non-0 is returned and *tgt is not modified.
    
    The error code cson_rc.IOError is used to indicate a db-level
    error, and cson_rc.ArgError is returned if the column count of the
    statement is 0 (indicating an invalid or non-SELECT statement).
    
    The JSON data types are determined by the column type as reported
    by cpdo column type:
    
    CPDO_TYPE_INT8...INT64: integer
    
    CPDO_TYPE_DOUBLE: double
    
    CPDO_TYPE_TEXT or CPDO_TYPE_BLOB: string, and this will only work if
    the data is UTF8 compatible. We treat BLOB as strings for the sake
    of MySQL, where fields of type TEXT are reported as BLOBs (at least
    by cpdo).

    If the db returns a literal or SQL NULL for a value it is converted
    to a JSON null. If it somehow finds a column type it cannot handle,
    the value is also converted to a NULL in the output.

    Example
    
    @code
    cson_value * json = NULL;
    int rc = cson_cpdo_stmt_to_json( myStatement, &json, 1 );
    if( 0 != rc ) { ... error ... }
    else {
        cson_output_FILE( json, stdout, NULL );
        cson_value_free( json );
    }
    @endcode
*/
int cson_cpdo_stmt_to_json( cpdo_stmt * st, cson_value ** tgt, char fat );

/**
    A convenience wrapper around cson_cpdo_stmt_to_json(), which
    takes SQL instead of a cpdo_stmt object. It has the same
    return value and argument semantics as that function.
*/
int cson_cpdo_sql_to_json( cpdo_driver * db, cson_value ** tgt, char const * sql, char fat );

/**
   Converts statement column names to a JSON Array of strings.

   st must be a valid prepared statement with columns (only fetching
   queries have them). This function creates a JSON Array containing
   its column names, in order.
    
   Returns a new Array value on success, which the caller owns.
   On error NULL is returned.
    
   st is not traversed or freed by this function - only the column
   count and names are read.
*/
cson_value * cson_cpdo_stmt_col_names( cpdo_stmt * st );

    
/**
   Converts a single row from a SELECT-style query to a JSON
   object. Each column in the statement is added to the new object as
   a key/value pair, with the key being the value returned by
   st->api->get.column_name().

   st MUST be a just-stepped statement for which the step()
   return value was CPDO_STEP_OK, or results are undefined.

   The exact type of each value in the returned object will be
   determined by matching up the statement-reported type as closely as
   possible with cson's JSON data types. If this is not possible,
   creation of a value will fail and NULL will be returned.
   
   Please see cpdo_stmt_api::cpdo_stmt_api_get::column_name() for
   important details regarding how column names "might" not be what
   the db user expects, and information on how to ensure that they are
   what one expects.
   
   On success it returns a new object containing the key/value
   pairs from the row. On error NULL is returned.

   Ownership of st is not modified.

   @see cson_cpdo_row_to_array()
 */
cson_value * cson_cpdo_row_to_object( cpdo_stmt * st );

/**
   Functionally almost identical to cson_cpdo_row_to_object(), the
   only difference being how the result objects gets its column names.
   st must be a freshly-step()'d handle holding a result row.
   colNames must be an Array with at least the same number of columns
   as st. If it has fewer, NULL is returned and this function has
   no side-effects.

   For each column in the result set, the colNames entry at the same
   index is used for the column key. If a given entry is-not-a String
   then conversion will fail and NULL will be returned.

   The one reason to prefer this over cson_cpdo_row_to_object() is
   that this one can share the keys across multiple rows (or even
   other JSON containers), whereas the former makes fresh copies of
   the column names for each row.
*/
cson_value * cson_cpdo_row_to_object2( cpdo_stmt * st, cson_array * colNames );

    
/**
   Similar to cson_cpdo_row_to_object(), but this function
   creates an Array and stores each value from st's current
   row into that array. Thus a statement with 5 columns will
   result in a 5-element array.

   st MUST be a just-stepped statement for which the step()
   return value was CPDO_STEP_OK, or results are undefined.

   See cson_cpdo_row_to_object() for more details about the
   SQL-to-JSON type conversions.

   On success it returns a new Array value. On error it returns
   NULL. (There are any number of error cases.)

   Ownership of st is not modified.

   @see cson_cpdo_row_to_object()
*/
cson_value * cson_cpdo_row_to_array( cpdo_stmt * st );

/**
    Tries to fetch the contents of 0-based column ndx from st
    as either a blob or string and parse it as JSON. On success
    0 is returned and *tgt will be pointed to the new JSON value,
    which the caller owns. It will be of type Object or Array.
    
    On error it returns non-0 (a cson_rc error code) and tgt is not
    modified.

    If the statement contains no string/blob value in the given field,
    but otherwise encounters no error, 0 is returned but *tgt is not
    modified. Thus the caller must be sure to assign it to NULL before
    calling this so that he can check for NULL when this function
    returns 0. If the given field can be extracted as neither string
    nor blob then cson_rc.TypeError is returned.

    Returns cson_rc.ArgError if either tgt or st are NULL.
*/
int cson_cpdo_parse_json( cpdo_stmt * st, uint16_t ndx, cson_value ** tgt );

/**
   Given a valid cpdo_stmt object, created as the result of a SELECT
   (or db-specific pseudo-select) statement, it converts the value in
   the given 0-based response column index to a JSON value and returns
   it as a new cson_value instance.

   On success, non-NULL is returned and the ownership of the value is
   transfered to the caller.

   On error NULL is returned. Errors include:

   - stmt is NULL or ndx is out of bounds for the statement.

   - Allocation error when creating the new JSON value.

   The cpdo-type-to-JSON-type conversions are as follows:

   - CPDO_TYPE_NULL = JSON null (not a literal NULL).

   - CPDO_TYPE_INT8..64: JSON integer

   - CPDO_TYPE_DOUBLE: JSON double

   - CPDO_TYPE_STRING or CPDO_TYPE_BLOB: a JSON string. Results are undefined
   if the value is stored in any encoding other than ASCII or UTF8.

   - CPDO_TYPE_CUSTOM is ASSUMED to be convertible to a string
   value. This is primarily to accommodate MySQL
   TIME/DATE/DATETIME/TIMESTAMP fields.
   
   Any other result will cause NULL to be returned.
*/
cson_value * cson_cpdo_stmt_to_value( cpdo_stmt * st, uint16_t ndx );

/**
   If jv is NULL or is-not-a Array/Object then
   st->api->bind.null(st,ndx) is called (and its value returned),
   otherwise...

   jv is serialized to a buffer and bound as a blob (preferred) or string
   (fallback, if blob bind fails) in the 1-based ndx column.

   Returns 0 on success or a cpdo_rc value on error.

   FIXME: the return value can currently be either a cson_rc or
   cpdo_rc value, and they have some ambiguity/overlap.
   
   FIXME: add a variant of this function which takes a client-provided
   cson_buffer object to (re)use for serialization purposes. Why?
   Because i want this in the whiki code :).

*/
int cson_cpdo_bind_json( cpdo_stmt * st, uint16_t ndx, cson_value const * jv );

/**
   Binds a JSON value to a db column (1-based). v must be NULL or one
   of the types (integer, double, bool, string, null). To bind Objects
   and Arrays as JSON strings use cson_cpdo_bind_json().

   Integers are _naively assumed_ to be bindable as 64-bit parameters
   (via st->api->bind.i64()). Booleans are treated as
   integers. Strings are bound as STRING fields, but if that fails we
   attempt to bind them as BLOBs instead. NULL and the cson null value
   are bound as SQL NULL.

   This function _might_ be changed in the future to pass Objects and Arrays
   to cson_cpdo_bind_json().

   Returns 0 on success and a cpdo_rc code on error. Returns
   cpdo_rc.ArgError if !st or !ndx and cpdo_rc.TypeError if v is not
   one of the above-listed types/values.
*/       
int cson_cpdo_bind_value( cpdo_stmt * st, uint16_t ndx, cson_value const * v );
    
#if defined(__cplusplus)
} /*extern "C"*/
#endif
    
#endif /* CSON_ENABLE_CPDO */
#endif /* WANDERINGHORSE_NET_CSON_CPDO_H_INCLUDED */
