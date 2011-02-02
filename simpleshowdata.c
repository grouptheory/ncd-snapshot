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
void showTableQuery(MYSQL *, int, int);
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


void showTableQuery(MYSQL *connread, int query_num, int limit_value)
{
      	if(connread == NULL) exit(1);

	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlbuffer[BIGBUFFER];
	char sqlstartbuffer[BIGBUFFER];
	
    fprintf(stderr,"Image\t%-50s\t\tImage\t%-50s\tNCD\tDouble NCD\n","File Number One", "File Number Two");
	
	snprintf(sqlstartbuffer, BIGBUFFER,"select a.image,CONCAT(a.directory,'/', a.filename), b.image, CONCAT(b.directory, '/', b.filename), ncd_normal, ncd_double FROM NCD_table AS n JOIN NCD_result AS r JOIN image_snapshot_table AS a JOIN image_snapshot_table AS b WHERE n.ncd_key = r.ncd_key AND n.file_one = a.item AND n.file_two = b.item AND n.querynumber = %d AND NOT file_one = file_two AND NOT a.filename = b.filename %s ORDER BY NCD_normal,NCD_double ", query_num, (GLOBAL_IDENT == 1) ? "AND CONCAT(a.directory,'/', a.filename) <> CONCAT(b.directory, '/', b.filename)" : "");
	if(limit_value > 0) snprintf(sqlbuffer,BIGBUFFER,"%s LIMIT %d;",sqlstartbuffer,limit_value);
	else snprintf(sqlbuffer,BIGBUFFER,"%s;",sqlstartbuffer);
	
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);

	while( (row = mysql_fetch_row(res)) != NULL)
	{
	    printf("%-4s\t%-20s\t%-4s\t%-20s\t%-7s\t%-7s\n", row[0],row[1],row[2],row[3],row[4],row[5]);
	}
}

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
  {"chunk", required_argument, NULL, 'c'},
  {"random", no_argument, NULL, 'o'},
  {"repeat", no_argument, NULL, 'k'},
  {"offset",  required_argument, NULL, 'O'},
  {"limit",  required_argument, NULL, 'l'},
  {"identical",  required_argument, NULL, 'i'},
  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("NCD Snapshot V.001 - Show the query result table.\n");
    printf("Defaults may be stored in your home directory in ./.snapshot.cnf\n");
    printf("Usage: ./simpleshowdata [OPTIONS] --query # \n");
    printf("\t-Q, --query-num\t Query number to use.\n");
	printf("\t-q, --query\t	Use the last query created.\n");
    printf("\t-H, --host=name\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-S, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
    printf("\t-f, --file=name\t Insted of MySQL - Dumps Insert statements into file: name\n");
    printf("\t-w, --show\t Show NCD Table when completed\n");
    printf("\t-l, --limit\t Limit the output of the show command.\n");
    printf("\t-i, --identical\t Show matches with same file name. (Default is not too).\n");   
    printf("\t-h, --help\t This help page.\n");
  
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
    MYSQL *connread = NULL;
    MYSQL *connwrite = NULL;
	char *query_number = NULL;
    char *investigator = NULL;
    char *case_name = NULL;
    char *image_name = NULL;
    int limit_value = 0;
  	int query_num = 0;  
    
  
    //Setup for MySQL Init File
    my_init();
    
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "H:u:p:P:Q:qSF:hnim:Nwl:", long_options, &option_index)) != EOF )
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
	  if(limit_value < 0) { fprintf(stdout,"Bad limit value entered.\n"); exit(1); }
	break;
		  
      }//switch      
     
    }//while
    
    //Move to end of Argument List
    argc -= optind; 
    argv += optind;
	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
	
     //Setup SQL  
     connread = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
     
	query_num = pickQuery(connread, query_number);

    showTableQuery(connread, query_num, limit_value);


     //Sql End
      mysql_close(connread);


    
    return(0);
}


