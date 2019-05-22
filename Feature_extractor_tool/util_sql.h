/*

    Vitor Hugo Galhardo Moia, Ph.D student
    Department of Computer Engineering and Industrial Automation (DCA)
    School of Electrical and Computer Engineering (FEEC)
    University of Campinas (UNICAMP)
    Campinas, SP, Brazil 13083-852
    Email: vhgmoia@dca.fee.unicamp.br / vitormoia@gmail.com
    page: http://www.dca.fee.unicamp.br/~vhgmoia/

    March, 28th 2019

    File: util_sql.h

*/

#include <sqlite3.h> 

/* Extractor tool */
//#define MRSH
#define SDHASH

#define SLEEP_TIME 2

sqlite3* open_connection(char *db_name);

void close_connection(sqlite3 *db);

void execute_sql_statement(char* sql, sqlite3 *db);

/* return the number of registers found */
int execute_sql_statement_select(char* sql, sqlite3 *db);

/* return the ID (integer) from a sql statement or -1 in case no register was found*/
int retrieve_id_using_sql_statmente(char* sql, sqlite3 *db);

const char *get_filename_ext(const char *filename);

size_t get_file_size(const char *fname);

int getting_id_from_objects_tb(const char* filename, sqlite3 *db);

int inserting_new_obj_into_objects_tb(const char* filename, const char* ext, size_t size, sqlite3 *db);

int getting_feature_id(char* hash, sqlite3 *db);

int inserting_new_feature(int id_obj, char* hash, int size_fet, char* offset, sqlite3 *db);

void updating_feature(int id_obj, int id_fet, char* offset, sqlite3 *db);

void insert_new_record_objs_vs_features(int id_obj, int id_fet, char* offset, sqlite3 *db);

void inserting_new_feature_single_tb(int id_obj, char* hash, int size_fet, char* offset, sqlite3 *db);

sqlite3_stmt *prepared_insert_feature_statement(sqlite3 *db);

void finalize_prepared_stmt(sqlite3_stmt *stmt);

void inserting_new_feature_prepared_stmt(int id_obj, char* hash, int size_fet, char* offset, sqlite3 *db, sqlite3_stmt *stmt);

void remove_existing_features(sqlite3 *db, int id_obj);
