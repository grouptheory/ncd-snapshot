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


void printhelp();


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
short SHOW_Start = 0;

void initTables(MYSQL *conn)
{
	if(conn == NULL) exit(1);
	
	char sqlbuffer[BIGBUFFER];
	
	fprintf(stderr,"Intializing Tables and Views  in Database\n");
	fprintf(stderr,"\tDropping table %s\n","Collabrative_Config");
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;", "Collabrative_Config");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n","Collabrative_Config");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_COLLABRATION_CONFIG);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	
	fprintf(stderr,"\tDropping table %s\n","Collabrative_Start");
	snprintf(sqlbuffer, BIGBUFFER,"DROP VIEW %s;", "Collabrative_Start");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n","Collabrative_Start");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_COLLABORATION_START);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	
	fprintf(stderr,"\tDropping view %s\n","Collabrative_Result");
	snprintf(sqlbuffer, BIGBUFFER,"DROP VIEW %s;", "Collabrative_Result");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating view %s\n","Collabrative_Result");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_COLLABORATION_RESULT);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
		
		
	fprintf(stderr,"Setting up default values\n");
	snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO Collabrative_Config (cutoff) VALUES (.5);");
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
  {"start", no_argument, NULL, 's'},
  {"limit",  required_argument, NULL, 'l'},
  {"identical",  required_argument, NULL, 'i'},
  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("NCD Snapshot V.001 - Sets Anomaly View and prints Similarity Score.\n");
    printf("Usage: ./collaboration [OPTIONS] \n");
    printf("\t-H, --host=name\t\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-S, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
    printf("\t-f, --file=name\t\t Insted of MySQL - Dumps Insert statements into file: name\n");
	printf("\t-I, --intialize\t\t Initialize new tables and views. (Warning: wipes/drops old).\n");
	printf("\t-C, --cutoff\t\t Change NCD cutoff value.\n");
	printf("\t-s, --start\t\t Show Collabrative_Start View\n");
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
    MYSQL *conn = NULL;
    int limit_value = 0;
	float cutoff_value = 0;
    char sqlbuffer[BIGBUFFER];
  
    //Setup for MySQL Init File
    my_init();
    
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "hh:IC:u:Q:qp:P:S:il:s", long_options, &option_index)) != EOF )
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
		case 'u' :
		  user_name = optarg;
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
		case 's' :
			SHOW_Start = 1;
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
		if(cutoff_value != 0)
		{
			snprintf(sqlbuffer,BIGBUFFER,"UPDATE Collabrative_Config SET cutoff = %f;",cutoff_value);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
		}

		if(SHOW_Start == 1) showTable(conn, "Collabrative_Start", limit_value);
	    showTable(conn, "Collabrative_Result", limit_value);
	}
     //Sql End
      mysql_close(conn);
    
    return(0);
}


