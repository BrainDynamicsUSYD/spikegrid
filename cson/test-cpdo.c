#if defined(NDEBUG)
#  undef NDEBUG /* force assert() to always work */
#endif
#include <assert.h>
#include <stdio.h>
#include <stdlib.h> /* atexit() */
#include <string.h> /* strlen() */
#include "wh/cson/cson_cpdo.h"

#define MARKER if(1) printf("MARKER: %s:%d:%s():\t",__FILE__,__LINE__,__func__); if(1) printf
#define SHOWRC(RC) MARKER("rc=%d (%s)\n",RC,cson_rc_string(RC))

static struct {
    cpdo_driver * db;
    char const * dsn;
    cson_output_opt outOpt;
} ThisApp = {
    NULL/*db*/,
#if 1
    "sqlite3::memory:"/*dsn*/,
#else
    "sqlite3:byoo.sqlite3"/*dsn*/,
#endif
    cson_output_opt_empty_m
};

static void setup_db()
{
    MARKER("db setup starting dsn=[%s]...\n", ThisApp.dsn);
    int rc = cpdo_driver_new_connect( &ThisApp.db, ThisApp.dsn, NULL, NULL);
    MARKER("connect rc=%d\n",rc);
    assert( (0 == rc) && "db open failed!" );

#define RUNSQL(SQL) \
    puts(SQL); \
    rc = cpdo_exec( ThisApp.db, SQL, strlen(SQL) );     \
    assert( (0 == rc) && "QUERY FAILED")

    char const * sql = "CREATE TABLE IF NOT EXISTS stuff ("
                        "name UNIQUE ON CONFLICT REPLACE, "
                        "count integer"
                        ");";
    RUNSQL(sql);
    RUNSQL("DELETE FROM stuff");
    RUNSQL("INSERT INTO stuff (name,count) VALUES('this',3)" );
    RUNSQL("INSERT INTO stuff (name,count) VALUES('that',2)" );
    RUNSQL("INSERT INTO stuff (name,count) VALUES('intZero',0)" );
    RUNSQL("INSERT INTO stuff (name,count) VALUES('doubleZero',0.0)" );
    RUNSQL("INSERT INTO stuff (name,count) VALUES('emptyString','')" );
    RUNSQL("INSERT INTO stuff (name,count) VALUES('nullValue',NULL)" );
#undef RUNSQL

    cpdo_stmt * st = NULL;
    sql = "INSERT INTO stuff (name,count) VALUES(?,?)";
    rc = cpdo_prepare( ThisApp.db, &st, sql, strlen(sql));
    assert((0==rc) && "Prepare failed.");

    /* Test cpdo_bind_value()... */
    cson_value * tmp = cson_value_new_string("hi!",3);
    rc = cson_cpdo_bind_value( st, 1, tmp );
    cson_value_free(tmp);
    assert(!rc && "Bind failed.");
    tmp = cson_value_new_double(47.73);
    rc = cson_cpdo_bind_value( st, 2, tmp );
    cson_value_free(tmp);
    assert(!rc && "Bind failed.");
    rc = st->api->step(st);
    assert( CPDO_STEP_DONE == rc );
    st->api->finalize(st);
    
    MARKER("db setup done.\n");
}


static void test1()
{
    MARKER("test1() starting.\n");
    assert( NULL != ThisApp.db );

    cpdo_stmt * st = NULL;
    char const * select = "SELECT * FROM stuff";
    int rc = cpdo_prepare( ThisApp.db, &st, select, strlen(select) );
    assert( 0 == rc );
    cson_value * json = NULL;
    char fat = 1;
    rc = cson_cpdo_stmt_to_json( st, &json, fat );
    st->api->finalize( st );
    assert( 0 == rc );
    assert( NULL != json );
    MARKER("Via cson_cpdo_stmt_to_json(%s mode) [%s]...\n",fat?"fat":"slim",select);
    cson_output_FILE( json, stdout, &ThisApp.outOpt );
    cson_value_free(json);
    json = NULL;
    fat = !fat;
    rc = cson_cpdo_sql_to_json( ThisApp.db, &json, select, fat);
    assert( 0 == rc );
    MARKER("Via cson_cpdo_sql_to_json(%s mode) [%s]...\n",fat?"fat":"slim",select);
    cson_output_FILE( json, stdout, &ThisApp.outOpt );
    cson_value_free(json);
    MARKER("test1() done.\n");
}

static void atexit_handler()
{
    if( NULL != ThisApp.db ) {
        cpdo_close(ThisApp.db);
        ThisApp.db = NULL;
    }
}
int main(int argc, char **argv)
{
    atexit( atexit_handler );
#if CPDO_ENABLE_SQLITE3
    cpdo_driver_sqlite3_register();
#endif
#if CPDO_ENABLE_MYSQL5
    cpdo_driver_mysql5_register();
#endif
    setup_db();
    ThisApp.outOpt.addNewline = 1;
    ThisApp.outOpt.indentation = 1;
    test1();
	return 0;
}
