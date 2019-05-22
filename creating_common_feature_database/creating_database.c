/*

    Vitor Hugo Galhardo Moia, Ph.D student
    Department of Computer Engineering and Industrial Automation (DCA)
    School of Electrical and Computer Engineering (FEEC)
    University of Campinas (UNICAMP)
    Campinas, SP, Brazil 13083-852
    Email: vhgmoia@dca.fee.unicamp.br / vitormoia@gmail.com
    page: http://www.dca.fee.unicamp.br/~vhgmoia/

    March, 28th 2019

    File: creating_database.c
    Purpose: Create a SQLite database with proper tables and indexes to store the common features

    INPUT: None.
    OUTPUT: Database with the name provided during execution.
*/

#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <string.h>
#include "util_sql.h"

int main(int argc, char* argv[]) {

	sqlite3 *db;
	char *sql;

	char nome_db[30];

	printf("Entre com o nome do banco a ser criado: ");
	fgets(nome_db, 30, stdin);

	printf("SIZE: %zu\n", strlen(nome_db));

	int i = strlen(nome_db) - 1;

	if(nome_db[ i ] == '\n')
		nome_db[ i ] = '\0';

	/* Open database */
	db = open_connection(nome_db);

	/* Create SQL statement */
	sql = "CREATE TABLE objects("  \
		"ID INTEGER PRIMARY KEY," \
		"NAME TEXT NOT NULL," \
		"EXTENSION TEXT,"\
		"SIZE INTEGER"\
		"NUM_FEAT INTEGER"\
		");";

	/* Execute SQL statement */
	execute_sql_statement(sql, db);

	/* Create SQL statement */
	sql = "CREATE TABLE features_sdhash("  \
		"ID_FEAT INTEGER PRIMARY KEY," \
		"ID_OBJ INTEGER NOT NULL," \
		"HASH TEXT NOT NULL," \
		"OFFSET TEXT,"\
		"SIZE_FEAT INTEGER,"\
		"FOREIGN KEY(ID_OBJ) REFERENCES objects(ID)"\
		");";

	/* Execute SQL statement */
	execute_sql_statement(sql, db);

	/* Create SQL statement */
	sql = "CREATE TABLE features_mrshv2("  \
		"ID_FEAT INTEGER PRIMARY KEY," \
		"ID_OBJ INTEGER NOT NULL," \
		"HASH TEXT NOT NULL," \
		"OFFSET TEXT,"\
		"SIZE_FEAT INTEGER,"\
		"FOREIGN KEY(ID_OBJ) REFERENCES objects(ID)"\
		");";

	/* Execute SQL statement */
	execute_sql_statement(sql, db);


	/* Create SQL statement */
	sql = "CREATE TABLE common_features_mrshv2("  \
		"HASH TEXT NOT NULL," \
		"CONT INTEGER,"\
		"CONT_DIFF INTEGER"\
		");";

	/* Execute SQL statement */
	execute_sql_statement(sql, db);

	/* Create SQL statement */
	sql = "CREATE TABLE common_features_sdhash("  \
		"HASH TEXT NOT NULL," \
		"CONT INTEGER,"\
		"CONT_DIFF INTEGER"\
		");";

	/* Execute SQL statement */
	execute_sql_statement(sql, db);

	sql = "CREATE INDEX idx_cc_features ON common_features_sdhash(HASH,CONT_DIFF);";
	/* Execute SQL statement */
	execute_sql_statement(sql, db);
	
	sql = "CREATE INDEX idx_features_sdhash ON features_sdhash(HASH, ID_OBJ);";
	/* Execute SQL statement */
	execute_sql_statement(sql, db);

	sql = "CREATE INDEX idx_features_sdhash_ID ON features_sdhash(ID_OBJ);";
	/* Execute SQL statement */
	execute_sql_statement(sql, db);

	sql = "CREATE INDEX idx_objects ON objects(ID, NAME);";
	/* Execute SQL statement */
	execute_sql_statement(sql, db);
   
	close_connection(db);	

	return 0;
}
