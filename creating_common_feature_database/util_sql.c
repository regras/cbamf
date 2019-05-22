/*

    Vitor Hugo Galhardo Moia, Ph.D student
    Department of Computer Engineering and Industrial Automation (DCA)
    School of Electrical and Computer Engineering (FEEC)
    University of Campinas (UNICAMP)
    Campinas, SP, Brazil 13083-852
    Email: vhgmoia@dca.fee.unicamp.br / vitormoia@gmail.com
    page: http://www.dca.fee.unicamp.br/~vhgmoia/

    March, 28th 2019

    File: util_sql.c
    Purpose: Provide functions to handle the database (SQLite)

*/

#include <sqlite3.h> 
#include <sys/stat.h>
#include <stdio.h>
#include "util_sql.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

sqlite3* open_connection(char *db_name){

	sqlite3 *db;
	int rc = sqlite3_open(db_name, &db);

	if(rc) {
		fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));	
	}
	return db;
}

void execute_sql_statement(char* sql, sqlite3 *db){

	char *zErrMsg = 0;
	int rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);

	if( rc != SQLITE_OK ) {
		if(rc == SQLITE_LOCKED || rc == SQLITE_BUSY){
			while(rc == SQLITE_LOCKED || rc == SQLITE_BUSY){
				sleep(SLEEP_TIME);
				rc = sqlite3_exec(db, sql, NULL, NULL, &zErrMsg);
			}
		}	
		else {
			fprintf(stderr, "SQL error (execute sql statement): %sSQL: %s\n", zErrMsg, sql);
      			sqlite3_free(zErrMsg);
		}

   	}
}

void close_connection(sqlite3 *db){

	int rc = sqlite3_close(db);
	char *zErrMsg = 0;

	if( rc != SQLITE_OK ) {
		fprintf(stderr, "SQL error (close connection): %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
	return;
}
