#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#include "mysql.h"

void mysql_print_error(MYSQL *conn)
{
 if(conn != NULL)
 {
   fprintf(stderr,"MySQL Error: %u : %s\n",mysql_errno(conn), mysql_error(conn));
 }
 else
   fprintf(stderr,"MySQL Error - No Handler\n");  
}

MYSQL *mysql_connect(char *hostname, char *username, char *password, char *db_name, unsigned int port_num, char *socket_name, unsigned int flags)
{
    MYSQL *conn;
    conn = mysql_init(NULL); //init Connection handler
    
    if(conn == NULL) {fprintf(stderr,"Error setting up MySQL Handler.\n"); exit(1);}
    if( mysql_real_connect(conn, hostname, username, password, db_name, port_num, socket_name, flags) == NULL )
    {
     mysql_print_error(conn);
     exit(1);
    }
      

    return conn;
}//mysql_connect

void showTable(MYSQL *connread, char *table, int limit_value)
{
    if(connread == NULL) exit(1);

	MYSQL_RES *res;
	MYSQL_ROW row;
	MYSQL_FIELD *field;
	int buffersize = 10000;
	char sqlbuffer[buffersize];
	char sql_temp_buffer[buffersize];
	my_ulonglong num_col;
	int count;
	//List Header
	printf("TABLE: %s\n",table);
	res = mysql_list_fields(connread, table, NULL);
	num_col = mysql_num_fields(res);

	while( (field = mysql_fetch_field(res)) != NULL)
	{
			printf("%s\t",field->name);
	}
	printf("\n");

	mysql_free_result(res);
	
	snprintf(sqlbuffer, buffersize,"select * FROM %s ",table);
	if(limit_value > 0) 
	{
		snprintf(sql_temp_buffer,buffersize," LIMIT %d ", limit_value);
		strncat(sqlbuffer, sql_temp_buffer, buffersize);
	}
	
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);

	while( (row = mysql_fetch_row(res)) != NULL)
	{
		for(count = 0; count < num_col; count++)
			printf("%s\t", row[count]);
		printf("\n");
	}
	printf("\n");
}

