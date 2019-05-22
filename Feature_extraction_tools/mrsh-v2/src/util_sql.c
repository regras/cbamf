/*
    Code adapted from sdhash (https://github.com/sdhash/sdhash)

    Vitor Hugo Galhardo Moia, Ph.D student
    Department of Computer Engineering and Industrial Automation (DCA)
    School of Electrical and Computer Engineering (FEEC)
    University of Campinas (UNICAMP)
    Campinas, SP, Brazil 13083-852
    Email: vhgmoia@dca.fee.unicamp.br / vitormoia@gmail.com
    page: http://www.dca.fee.unicamp.br/~vhgmoia/

    March, 28th 2019

    File: util_sql.c

*/

#include <sqlite3.h> 
#include <sys/stat.h>
#include <stdio.h>
#include "../header/util_sql.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
	int i;

	if(argc > 0){
	   	for(i = 0; i<argc; i++) {
	      		printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
	   	}
	   	printf("\n");
	}
	else
		printf("No record!\n");

   	return 0;
}


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

int retrieve_id_using_sql_statmente(char* sql, sqlite3 *db){

	char *zErrMsg = 0;
	sqlite3_stmt *stmt; 
	int result = 0;
	int id=-1;

	result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

	if( result == SQLITE_OK ) {
		if ( sqlite3_step(stmt) == SQLITE_ROW ){
			id = sqlite3_column_int (stmt, 0);
			sqlite3_finalize(stmt);
			return id;
		} 
   	}
	else{
		if(result == SQLITE_LOCKED || result == SQLITE_BUSY){
			while(result == SQLITE_LOCKED || result == SQLITE_BUSY){
				sleep(SLEEP_TIME);
				result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

				if( result == SQLITE_OK ) {
					if ( sqlite3_step(stmt) == SQLITE_ROW ){
						id = sqlite3_column_int (stmt, 0);
						sqlite3_finalize(stmt);
						return id;
					} 
			   	}	
			}
		}	
		else {

			fprintf(stderr, "SQL error (retrieve id): %s\n", zErrMsg);
      			sqlite3_free(zErrMsg);
		}
	}

	sqlite3_finalize(stmt);

	return id;
}

int execute_sql_statement_select(char* sql, sqlite3 *db){
   
	char *zErrMsg = 0;
	sqlite3_stmt *stmt; /* pointing to a compiled prepared statement that can be executed using sqlite3_step() */
	int result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL); //compile into a byte-code program
	/* NULL = When sql has more than one statement, you can receive a pointer to the beginning of the next statement. https://stackoverflow.com/questions/11643325/why-we-use-null-in-sqlite3-prepare-v2 */
	int row = 0;

	if( result == SQLITE_OK ) {
		if ( sqlite3_step(stmt) == SQLITE_ROW ){
			do
			{
				for(int i=0; i < sqlite3_column_count(stmt); i++){
					const unsigned char * text = sqlite3_column_text (stmt, i); /* (pointer to the prepared statement, index of the column) */
					//printf ("%s | ", text);
				}
				//printf("\n");
				row++;
			}while(sqlite3_step(stmt) == SQLITE_ROW);
		} 
   	}
	else{
		if(result == SQLITE_LOCKED || result == SQLITE_BUSY ){
			while(result == SQLITE_LOCKED || result == SQLITE_BUSY){
				sleep(SLEEP_TIME);
				result = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);

				if ( sqlite3_step(stmt) == SQLITE_ROW ){
					do
					{
						for(int i=0; i < sqlite3_column_count(stmt); i++){
							const unsigned char * text = sqlite3_column_text (stmt, i); /* (pointer to the prepared statement, index of the column) */
						}
						row++;
					}while(sqlite3_step(stmt) == SQLITE_ROW);
				} 	
			}
		}	
		else {

			fprintf(stderr, "SQL error (select statement): %s\n", zErrMsg);
      			sqlite3_free(zErrMsg);
		}
	}

	sqlite3_finalize(stmt);

	return row;
}

void close_connection(sqlite3 *db){

	int rc = sqlite3_close(db);
	char *zErrMsg = 0;

	if( rc != SQLITE_OK ) {
		fprintf(stderr, "SQL error (close connection): %s\n", zErrMsg);
		sqlite3_free(zErrMsg);
	}
}


const char *get_filename_ext(const char *filename) {
	const char *dot = strrchr(filename, '.');
	if(!dot || dot == filename) return "exe";
	return dot + 1;
}

size_t get_file_size(const char *fname) {

  struct stat st;
  if(stat(fname, &st) == -1)
  {
    fprintf(stderr, "Cannot stat %s", fname);
    perror("");
    exit(1);
  }

  return st.st_size;
}


/* ******** BUILDING SQL STATEMENTS ******** */

int getting_id_from_objects_tb(char* filename, sqlite3 *db){
	
	char sql[150];
	sql[0]='\0';
	sprintf(sql, "SELECT ID from objects where NAME='%s'", filename);

	return retrieve_id_using_sql_statmente(sql, db);
}

int inserting_new_obj_into_objects_tb(char* filename, char* ext, size_t size, sqlite3 *db){

	char sql[150];
	sql[0]='\0';
	sprintf(sql, "INSERT INTO objects (NAME, EXTENSION, SIZE) VALUES ('%s', '%s', %zu)", filename, ext, size);
	execute_sql_statement(sql, db);

	return getting_id_from_objects_tb(filename, db);
}

int getting_feature_id(char* hash, sqlite3 *db){

	char sql[400];
	sql[0]='\0';

	#ifdef MRSH
	sprintf(sql, "SELECT ID from features_mrshv2 where HASH='%X'", hash);
	#endif
	#ifdef SDHASH
	sprintf(sql, "SELECT ID from features_sdhash where HASH='%X'", hash);
	#endif

	return retrieve_id_using_sql_statmente(sql, db);

}

int inserting_new_feature(int id_obj, char* hash, int size_fet, char* offset, sqlite3 *db){

	char sql[200];
	sql[0]='\0';

	#ifdef MRSH
	sprintf(sql, "INSERT INTO features_mrshv2 (HASH, COUNT, SIZE_FEAT) VALUES ('%X', 1, '%d')", hash, size_fet);
	#endif
	#ifdef SDHASH
	sprintf(sql, "INSERT INTO features_sdhash (HASH, COUNT, SIZE_FEAT) VALUES ('%s', 1, '%d')", hash, size_fet);
	#endif

	execute_sql_statement(sql, db);

	int id_fet = getting_feature_id(hash, db);

	if(id_fet >= 0) {
		/* Creating a new record in objects vs features table */
		insert_new_record_objs_vs_features(id_obj, id_fet, offset, db);
	}

	return id_fet;
}

void updating_feature(int id_obj, int id_fet, char* offset, sqlite3 *db) {

	/* Checking if the feature already exists */
	char sql_exists[200];
	sql_exists[0]='\0';

	#ifdef MRSH
	sprintf(sql_exists, "select features_mrshv2.ID from features_mrshv2 inner join objects_vs_features_mrshv2 ON features_mrshv2.ID = objects_vs_features_mrshv2.ID_FEAT where features_mrshv2.ID = '%d' AND objects_vs_features_mrshv2.OFFSET = '%X' AND objects_vs_features_mrshv2.ID_OBJ = %d;", id_fet, offset, id_obj);
	#endif
	#ifdef SDHASH
	sprintf(sql_exists, "select features_sdhash.ID from features_sdhash inner join objects_vs_features_sdhash ON features_sdhash.ID = objects_vs_features_sdhash.ID_FEAT where features_sdhash.ID = '%d' AND objects_vs_features_sdhash.OFFSET = '%X' AND objects_vs_features_sdhash.ID_OBJ = %d;", id_fet, offset, id_obj);
	#endif

	int rows = execute_sql_statement_select(sql_exists, db);

	if( rows <= 0 )
	{
		char sql_select[200];
		sql_select[0]='\0';
		#ifdef MRSH
		sprintf(sql_select, "SELECT COUNT from features_mrshv2 WHERE ID = %d", id_fet);
		#endif
		#ifdef SDHASH
		sprintf(sql_select, "SELECT COUNT from features_sdhash WHERE ID = %d", id_fet);
		#endif

		/* this function will return an integer, just like ID */
		int count = retrieve_id_using_sql_statmente(sql_select, db);

		if(count > 0){
			char sql_update[200];
			sql_update[0]='\0';
			#ifdef MRSH
			sprintf(sql_update, "UPDATE features_mrshv2 SET COUNT = %d WHERE ID = %d", count+1, id_fet);
			#endif

			#ifdef SDHASH
			sprintf(sql_update, "UPDATE features_sdhash SET COUNT = %d WHERE ID = %d", count+1, id_fet);
			#endif

			//printf("\nSQL UPDATE: %s\n", sql_update);
			execute_sql_statement(sql_update, db);

			/* Creating a new record in objects vs features table */
			insert_new_record_objs_vs_features(id_obj, id_fet, offset, db);
		}
	}
}

void insert_new_record_objs_vs_features(int id_obj, int id_fet, char* offset, sqlite3 *db){

	char sql[200];
	sql[0]='\0';

#ifdef MRSH
	sprintf(sql, "INSERT INTO objects_vs_features_mrshv2 (ID_OBJ, ID_FEAT, OFFSET) VALUES (%d, %d, '%X')", id_obj, id_fet, offset);
#endif

#ifdef SDHASH
	sprintf(sql, "INSERT INTO objects_vs_features_sdhash (ID_OBJ, ID_FEAT, OFFSET) VALUES (%d, %d, '%X')", id_obj, id_fet, offset);
#endif

	execute_sql_statement(sql, db);
}

void inserting_new_feature_single_tb(int id_obj, char* hash, int size_fet, char* offset, sqlite3 *db){

	char sql[200];
	sql[0]='\0';

	#ifdef MRSH
	sprintf(sql, "INSERT INTO features_mrshv2 (ID_OBJ, HASH, OFFSET, SIZE_FEAT) VALUES ('%d', '%s', '%s', '%d')", id_obj, hash, offset, size_fet);
	#endif
	#ifdef SDHASH
	sprintf(sql, "INSERT INTO features_sdhash (ID_OBJ, HASH, OFFSET, SIZE_FEAT) VALUES ('%d', '%s', '%s', '%d')", id_obj, hash, offset, size_fet);
	#endif

	execute_sql_statement(sql, db);
}

sqlite3_stmt* prepared_insert_feature_statement(sqlite3 *db){

	char sql[200];
	sql[0]='\0';

	sqlite3_stmt *stmt;

	#ifdef MRSH
	//sprintf(sql, "INSERT INTO features_mrshv2 (HASH, COUNT, SIZE_FEAT) VALUES ('%X', 1, '%d')", hash, size_fet);
	strcpy(sql, "INSERT INTO features_mrshv2 (ID_OBJ, HASH, OFFSET, SIZE_FEAT) VALUES (?,?,?,?)");	

	#endif
	#ifdef SDHASH
	//sprintf(sql, "INSERT INTO features_sdhash (ID_OBJ, HASH, OFFSET, SIZE_FEAT) VALUES ('%d', '%s', '%s', '%d')", id_obj, hash, offset, size_fet);
	strcpy(sql, "INSERT INTO features_sdhash (ID_OBJ, HASH, OFFSET, SIZE_FEAT) VALUES (?,?,?,?)");
	#endif
	
	if ( sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
		printf("\nCould not prepare statement.");
    		return NULL;
  	}

	return stmt;
	
}

void finalize_prepared_stmt(sqlite3_stmt *stmt){

	sqlite3_finalize(stmt);
}

void inserting_new_feature_prepared_stmt(int id_obj, char* hash, int size_fet, char* offset, sqlite3 *db, sqlite3_stmt *stmt){

	//Binding parameters...

	if (sqlite3_bind_int(stmt, 1, id_obj) != SQLITE_OK) {
    		printf("\nCould not bind int.\n");
    		return;
  	}

	if (sqlite3_bind_text(stmt, 2, hash, strlen(hash), SQLITE_TRANSIENT) != SQLITE_OK) {
	    	printf("\nCould not bind text.\n");
	    	return;
	}

	if (sqlite3_bind_text(stmt, 3, offset, strlen(offset), SQLITE_TRANSIENT) != SQLITE_OK) {
	    	printf("\nCould not bind text.\n");
	    	return;
	}

	if (sqlite3_bind_int(stmt, 4, size_fet) != SQLITE_OK) {
    		printf("\nCould not bind int.\n");
    		return;
  	}

	//Executing sql statement...
	
	if (sqlite3_step(stmt) != SQLITE_DONE) {
    		printf("\nCould not step (execute) stmt.\n");
    		return;
  	}

	//Cleaning parameters values...

  	sqlite3_reset(stmt);
}

void remove_existing_features(sqlite3 *db, int id_obj){

	char sql[150];
	sql[0]='\0';
	#ifdef MRSH
	sprintf(sql, "DELETE FROM features_mrshv2 where ID_OBJ=%d", id_obj);
	#endif
	#ifdef SDHASH
	sprintf(sql, "DELETE FROM features_sdhash where ID_OBJ=%d", id_obj);
	#endif

	execute_sql_statement(sql, db);
}

sqlite3_stmt* prepared_statement_select_num_features(sqlite3 *db){

    char sql[200];
    sql[0]='\0';

    sqlite3_stmt *stmt;

    strcpy(sql, "SELECT NUM_FEAT FROM objects WHERE NAME=?");

    if ( sqlite3_prepare_v2(db, sql, -1, &stmt, NULL) != SQLITE_OK) {
        printf("\nCould not prepare statement.");
        return NULL;
    }

    return stmt;
}


int return_num_features_object_by_name(char* name, sqlite3_stmt* smtp){

    if (sqlite3_bind_text(smtp, 1, name, strlen(name), SQLITE_TRANSIENT) != SQLITE_OK) {
        printf("\nCould not bind text.\n");
        return 0;
    }

    int num_f = 0;

    if ( sqlite3_step(smtp) == SQLITE_ROW ){
        do
        {
            num_f = sqlite3_column_int(smtp, 0);

        }while(sqlite3_step(smtp) == SQLITE_ROW);
    }

    sqlite3_reset(smtp);

    return num_f;
}
