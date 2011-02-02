#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
#include <getopt.h>
#include "mysql.h"


#define BIGBUFFER 10000
#define sqlbufsize 255
#define db_name "SNAPSHOT"
#define DRIVE_TABLENAME "image_snapshot_table"
#define IMAGE_LIST_TABLENAME "image_list_table"
#define NCD_TABLENAME "NCD_table"
#define QUERY_TABLENAME "query_table"
#define NCD_RESULT_TABLENAME "NCD_result"


int checkfile(const char *);
void printhelp();
int pickQuery(MYSQL *, char *);

//Flags
int GLOB_INIT_FLAG = 0; //Initialize Tables - Default NO
int GLOBAL_VERBOSE = 0;
int GLOBAL_SHOWMATCHING = 0;
int GLOBAL_IDENT = 0;
int GLOBAL_CVSOUT = 0;
float GLOBAL_BASE = 0;
int GLOBAL_RANDOM = 0;
int GLOBAL_RANDOMK = 0;
int GLOBAL_SHOWOUT = 1;
int GLOBAL_NEWQUERY = 0;
char cursor[4]={'/','-','\\','|'}; //For Spinning Cursor
short SHOW_AnomalyCurve = 0;
short SHOW_AnomalyQueryStart = 0;
short SHOW_AnomalyQueryNumber = 0;
short SHOW_AnomalyJoin = 0;


int pickQuery(MYSQL *conn, char *query_number)
{
   	if(conn == NULL) exit(1);

	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlbuffer[BIGBUFFER];
	char second_sqlbuffer[BIGBUFFER];
	char tempstring[20];
	char query_name[sqlbufsize];
	int query_num = 0;
	int check = 0;
	my_ulonglong num_rows;

	
	if(GLOBAL_NEWQUERY == 1)
	{
		snprintf(sqlbuffer, BIGBUFFER,"select MAX(querynumber) FROM %s;",NCD_TABLENAME);
		if (mysql_query(conn,sqlbuffer) != 0)
			mysql_print_error(conn);
		res = mysql_use_result(conn);
		if( ((row = mysql_fetch_row(res)) != NULL) && row[0] != NULL)
			{
				query_num = atoi(row[0]);
				check = 1;
			}
		else 
		{
			fprintf(stderr,"No Queries Found! Nothing to do.\n");
			exit(-1);
		} 
		while( (row = mysql_fetch_row(res)) != NULL);
	}
	
	
	//Check provided number
	if(query_number != NULL)
	if( (query_num = atoi(query_number)) > 0 )
	{//perform Check
		snprintf(sqlbuffer, BIGBUFFER,"select DISTINCT querynumber FROM %s WHERE querynumber = %d",NCD_TABLENAME, query_num);
		if (mysql_query(conn,sqlbuffer) != 0)
			mysql_print_error(conn);
		res = mysql_store_result(conn);
		num_rows = mysql_num_rows(res);
		if(num_rows == 1) 
		{ 
			check = 1; 
			while( (row = mysql_fetch_row(res)) != NULL);
		}
		else 
		{
			printf("Error with intial query value [%d] provided.\n",query_num);
			while( (row = mysql_fetch_row(res)) != NULL); //goto end
		}
	}
	
	
	while(check == 0)
	{
		printf("\nPlease enter query number [L][N]][?]: ");
		fgets(tempstring,18,stdin);
		query_num = atoi(tempstring);
		if(query_num > 0) //Number
		{	//perform check
			snprintf(sqlbuffer, BIGBUFFER,"select DISTINCT querynumber FROM %s WHERE querynumber = %d",NCD_TABLENAME,query_num);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
			res = mysql_store_result(conn);
			num_rows = mysql_num_rows(res);
			if(num_rows == 1) 
			{ 
				check = 1; 
				while( (row = mysql_fetch_row(res)) != NULL);
			}
			else 
			{
				printf("Error with query value [%d] provided.\n",query_num);
				while( (row = mysql_fetch_row(res)) != NULL); //goto end			
			}
		}

		else if ( !strncasecmp(tempstring, "L", 1) )
		{
			//List Current Queriees
			snprintf(sqlbuffer, BIGBUFFER,"select * FROM %s",QUERY_TABLENAME);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
			res = mysql_store_result(conn);
			printf("query#\tCase#\tTimeT\tSizeT\tUnit\t MAC S Compare\n");
			while( (row = mysql_fetch_row(res)) != NULL)
				{
					printf("%-5s\t%-5s\t%-7s\t%-7s\t%-9s%-1s%-1s%-1s %-1s %-7s\n", row[0],row[1],row[2],row[3],row[4],row[5],row[6],row[7],row[8],row[9]);
				}
		}
		else if (!strncasecmp(tempstring, "?", 1) )
		{
			printf("Help:\nEnter number of a query. \nEnter [L] for List of current querys. \n"); 		
		}	
	}//While
	
	return query_num;
}


void initTables(MYSQL *conn)
{
	if(conn == NULL) exit(1);
	
	char sqlbuffer[BIGBUFFER];
	
	fprintf(stderr,"Intializing Tables and Views  in Database\n");
	fprintf(stderr,"\tDropping table %s\n","AnomalyConfig");
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;", "AnomalyConfig");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n","AnomalyConfig");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyConfig);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	
	fprintf(stderr,"\tDropping table %s\n","AnomalyCurve");
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;", "AnomalyCurve");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n","AnomalyCurve");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyCurve);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	
	fprintf(stderr,"\tDropping view %s\n","AnomalyQueryStart");
	snprintf(sqlbuffer, BIGBUFFER,"DROP VIEW %s;", "AnomalyQueryStart");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating view %s\n","AnomalyQueryStart");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyQueryStart);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
		
	fprintf(stderr,"\tDropping view %s\n","AnomalyQueryNumber");
	snprintf(sqlbuffer, BIGBUFFER,"DROP VIEW %s;", "AnomalyQueryNumber");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating view %s\n","AnomalyQueryNumber");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyQueryNumber);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	

	fprintf(stderr,"\tDropping view %s\n","AnomalyQueryNumber");
	snprintf(sqlbuffer, BIGBUFFER,"DROP VIEW %s;", "AnomalyQueryNumber");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating view %s\n","AnomalyQueryNumber");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyQueryNumber);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);			
	
	fprintf(stderr,"\tDropping view %s\n","AnomalyJoin");
	snprintf(sqlbuffer, BIGBUFFER,"DROP VIEW %s;", "AnomalyJoin");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating view %s\n","AnomalyJoin");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyJoin);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tDropping view %s\n","AnomalyImageSimilarityScore");
	snprintf(sqlbuffer, BIGBUFFER,"DROP VIEW %s;", "AnomalyImageSimilarityScore");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating view %s\n","AnomalyImageSimilarityScore");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyImageSimilarityScore);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
		
	fprintf(stderr,"Setting up default values\n");
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO AnomalyCurve (number, anomaly) VALUES (0, 0);");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO AnomalyCurve (number, anomaly) VALUES (1, 0.01);");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO AnomalyCurve (number, anomaly) VALUES (2, 0.02);");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO AnomalyCurve (number, anomaly) VALUES (3, 0.03);");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO AnomalyCurve (number, anomaly) VALUES (4, 0.04);");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO AnomalyCurve (number, anomaly) VALUES (5, 0.05);");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO AnomalyConfig (cutoff, query) VALUES (0, 0);");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

}


static const char *groups[] = {"client", 0};

struct option long_options[] =
{
  {"host", required_argument, NULL, 'H'},
  {"user", required_argument, NULL, 'u'},
  {"password", required_argument, NULL, 'p'},
  {"port", required_argument, NULL, 'P'},
  {"socket", required_argument, NULL, 'S'},
  {"query-num", required_argument, NULL, 'Q'},
  {"query", no_argument, NULL, 'q'},
  {"help", no_argument, NULL, 'h'},
  {"intialize", no_argument, NULL, 'I'},
  {"cutoff",  required_argument, NULL, 'C'},
  {"curve", no_argument, NULL, 'c'},
  {"start", no_argument, NULL, 's'},
  {"number", no_argument, NULL, 'n'},
  {"join", no_argument, NULL, 'j'},
  {"limit",  required_argument, NULL, 'l'},
  {"identical",  required_argument, NULL, 'i'},
  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("NCD Snapshot V.001 - Sets Anomaly View and prints Similarity Score.\n");
    printf("Usage: ./anomaly [OPTIONS] \n");
    printf("\t-Q, --query-num\t\t Query number to use.\n");
	printf("\t-q, --query\t	 Use the last query created.\n");
    printf("\t-H, --host=name\t\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-S, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
    printf("\t-f, --file=name\t\t Insted of MySQL - Dumps Insert statements into file: name\n");
	printf("\t-I, --intialize\t\t Initialize new tables and views. (Warning: wipes/drops old).\n");
	printf("\t-C, --cutoff\t\t Change NCD cutoff value.\n");
    printf("\t-c, --curve\t\t Show AnomalyCurve Table\n");
	printf("\t-s, --start\t\t Show AnomalyQueryStart View\n");
	printf("\t-n, --number\t\t Show AnomalyQueryNumber View\n");
	printf("\t-j, --join\t\t Show AnomalyJoin View\n");
    printf("\t-l, --limit\t\t Limit the output of the show command.\n");
    printf("\t-h, --help\t\t This help page.\n");
  
}
#define PASS_SIZE 100
int main(int argc, char *argv[] )
{
  
    struct dirDFS *directory;
    char *host_name = NULL;
    char *user_name = NULL;
    char *password = NULL;
    char tempstring[20];
    char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
    unsigned int port_num = 0;
    char *socket_name = NULL;
    int input,option_index,key;
    char scandir[PATH_MAX];
    FILE *fileoutput = NULL;
    char OUTPUT_PATH[PATH_MAX];
    MYSQL *conn = NULL;
	char *query_number = NULL;
    char *investigator = NULL;
    char *case_name = NULL;
    char *image_name = NULL;
    int limit_value = 0;
	float cutoff_value = 0;
  	int query_num = 0;  
    char sqlbuffer[BIGBUFFER];
  
    //Setup for MySQL Init File
    my_init();
    
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "hh:IC:m:u:Q:qp:P:S:il:csnj", long_options, &option_index)) != EOF )
    {
      switch(input)
      {
		case 'h' :
		    printhelp();
		    exit(1);
		  break;
		case 'H' :
		  host_name = optarg;
		  break;
		case 'I' :
		  GLOB_INIT_FLAG = 1;
		  break;
		case 'C' :
			strncpy(tempstring,optarg,19);
			cutoff_value = atof(tempstring);
			if(cutoff_value <= 0) { fprintf(stdout,"Bad limit value entered.\n"); exit(1); }
			break;
		case 'm' :
		  image_name = optarg;
		  break;
		case 'u' :
		  user_name = optarg;
		  break;	  
		case 'Q' :
		  query_number = optarg;
		  break;
		case 'q' :
		  GLOBAL_NEWQUERY = 1;
		  break;
		case 'p' :
		  snprintf(passbuffer,PASS_SIZE,"%s\0",optarg);
		  password = passbuffer; // no more NULL
		  //Neat trick from MySQL C API Book - Online to hide password from PS
		  while (*optarg) *optarg++ = ' ';
		  
		  break;	
		case 'P' :
		  port_num = (unsigned int) atoi(optarg);
		  break;
		case 'S' :
		  socket_name = optarg;  
		  break;
		case 'i' :
		  GLOBAL_IDENT = 1;
		  break;
		case 'l' :
		  strncpy(tempstring,optarg,19);
		  limit_value = atoi(tempstring);
		  if(limit_value <= 0) { fprintf(stdout,"Bad limit value entered.\n"); exit(1); }
		break;
		case 'c' :
			SHOW_AnomalyCurve = 1;
			break;
		case 's' :
			SHOW_AnomalyQueryStart = 1;
			break;
		case 'n' :
			SHOW_AnomalyQueryNumber = 1;
			break;
		case 'j' :
			SHOW_AnomalyJoin = 1;
			break;

			  
	    }//switch      
     
    }//while
    
    //Move to end of Argument List
    argc -= optind; 
    argv += optind;
	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
	
     //Setup SQL  
     conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
     
	 
	
	if(GLOB_INIT_FLAG == 1)
	{
		initTables(conn);
	}  
	else 
	{
		query_num = pickQuery(conn, query_number);
		//update Query
		snprintf(sqlbuffer,BIGBUFFER,"UPDATE AnomalyConfig SET query = %d;",query_num);
		if (mysql_query(conn,sqlbuffer) != 0)
			mysql_print_error(conn);
		
		if(cutoff_value != 0)
		{
			snprintf(sqlbuffer,BIGBUFFER,"UPDATE AnomalyConfig SET cutoff = %f;",cutoff_value);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
		}

		if(SHOW_AnomalyCurve == 1) showTable(conn, "AnomalyCurve", limit_value);
		if(SHOW_AnomalyQueryStart == 1) showTable(conn, "AnomalyQueryStart", limit_value);
		if(SHOW_AnomalyQueryNumber == 1) showTable(conn, "AnomalyQueryNumber", limit_value);
		if(SHOW_AnomalyJoin == 1) showTable(conn, "AnomalyJoin", limit_value);		
	    showTable(conn, "AnomalyImageSimilarityScore", limit_value);
	}
     //Sql End
      mysql_close(conn);
    
    return(0);
}


