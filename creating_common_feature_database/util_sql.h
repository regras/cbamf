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

#define SLEEP_TIME 2

sqlite3* open_connection(char *db_name);

void close_connection(sqlite3 *db);

void execute_sql_statement(char* sql, sqlite3 *db);
