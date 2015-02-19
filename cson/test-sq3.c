#if defined(NDEBUG)
#  undef NDEBUG /* force assert() to always work */
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* atexit() */
#include "wh/cson/cson_sqlite3.h"

#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf
#define SHOWRC(RC) MARKER("rc=%d (%s)\n",RC,cson_rc_string(RC))

static struct {
    sqlite3 * db;
    char const * fname;
    cson_output_opt outOpt;
} ThisApp = {
    NULL/*db*/,
    ":memory:"/*fname*/,
    cson_output_opt_empty_m
};

static void setup_db()
{
    MARKER("db setup starting [file=%s]...\n", ThisApp.fname);
    int rc = sqlite3_open_v2( ThisApp.fname, &ThisApp.db, 
                            SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                            NULL );
    assert( (0 == rc) && "db open failed!" );

#define RUNSQL(SQL) \
    puts(SQL); \
    rc = sqlite3_exec( ThisApp.db, SQL, NULL, NULL, NULL ); \
    assert( (0 == rc) && "QUERY FAILED")

    char const * sql = "CREATE TABLE IF NOT EXISTS stuff ("
                        "name UNIQUE ON CONFLICT REPLACE, "
                        "count tinyint(1)"
                        ");";
    RUNSQL(sql);
    RUNSQL("DELETE FROM stuff");
    RUNSQL("INSERT INTO STUFF (name,count) VALUES('this',3)" );
    RUNSQL("INSERT INTO STUFF (name,count) VALUES('that',2)" );
    RUNSQL("INSERT INTO STUFF (name,count) VALUES('intZero',0)" );
    RUNSQL("INSERT INTO STUFF (name,count) VALUES('doubleZero',0.0)" );
    RUNSQL("INSERT INTO STUFF (name,count) VALUES('emptyString','')" );
    RUNSQL("INSERT INTO STUFF (name,count) VALUES('nullValue',NULL)" );
#undef RUNSQL

    cson_value * x = cson_value_new_string("hi!",3);
    cson_value * y = cson_value_new_integer(42);
    cson_array * ar = cson_new_array();
    cson_array_append(ar, x);
    cson_array_append(ar, y);
    sqlite3_stmt * st = NULL;
    rc = sqlite3_prepare_v2( ThisApp.db, "INSERT INTO STUFF(name,count) VALUES(?,?)",
                             -1, &st, NULL);
    assert(SQLITE_OK == rc);
    cson_sqlite3_bind_value( st, 1, cson_array_value(ar) );
    cson_free_array(ar);
    rc = sqlite3_step(st);
    assert(SQLITE_DONE==rc);
    sqlite3_finalize(st);
    MARKER("db setup done.\n");
}


static void test1()
{
    MARKER("test1() starting.\n");
    assert( NULL != ThisApp.db );
    
    sqlite3_stmt * st = NULL;
    char const * select = "SELECT * FROM stuff";
    int rc = sqlite3_prepare_v2( ThisApp.db, select, -1,
            &st, NULL );
    assert( 0 == rc );
    cson_value * json = NULL;
    char fat = 1;
    rc = cson_sqlite3_stmt_to_json( st, &json, fat );
    sqlite3_finalize(st);
    assert( 0 == rc );
    assert( NULL != json );
    MARKER("Via cson_sqlite3_stmt_to_json(%s mode) [%s]...\n",fat?"fat":"slim",select);
    cson_output_FILE( json, stdout, &ThisApp.outOpt );
    cson_value_free(json);
    json = NULL;
    fat = !fat;
    rc = cson_sqlite3_sql_to_json( ThisApp.db, &json, select, fat);
    assert( 0 == rc );
    MARKER("Via cson_sqlite3_sql_to_json(%s mode) [%s]...\n",fat?"fat":"slim",select);
    cson_output_FILE( json, stdout, &ThisApp.outOpt );
    cson_value_free(json);
    MARKER("test1() done.\n");
}

static void atexit_handler()
{
    if( NULL != ThisApp.db ) {
        sqlite3_close(ThisApp.db);
        ThisApp.db = NULL;
    }
}
int main(int argc, char **argv)
{
    atexit( atexit_handler );
    setup_db();
    ThisApp.outOpt.addNewline = 1;
    ThisApp.outOpt.indentation = 0;
    test1();
	return 0;
}
