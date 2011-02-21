#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
#include <getopt.h>
#include "mysql.h"

#define ARRAYSIZE_TIME 1000
#define ARRAYSIZE_IMAGES 20
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
int MODVALUE = ARRAYSIZE_IMAGES;
float CUTOFF = 0.5;
char cursor[4]={'/','-','\\','|'}; //For Spinning Cursor
short SHOW_Start = 0;
float array[ARRAYSIZE_IMAGES][ARRAYSIZE_IMAGES][ARRAYSIZE_TIME];




void initTables(MYSQL *conn, int time)
{
	if(conn == NULL) exit(1);
	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlbuffer[BIGBUFFER];
	
	snprintf(sqlbuffer,BIGBUFFER,"UPDATE AnomalyConfig SET query = %d;",time);
		if (mysql_query(conn,sqlbuffer) != 0)
			mysql_print_error(conn);
	
	if(GLOB_INIT_FLAG) fprintf(stderr,"\tCreating temporary table %s\n","Collaborative_Start_Temp");
	snprintf(sqlbuffer,BIGBUFFER,"DROP TABLE IF EXISTS Collaborative_Start_Temp;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE Collaborative_Start_Temp AS SELECT DISTINCT S.IMG1 AS IMG1, S.F1 AS F1, J.anomaly AS A1, S.IMG2 AS IMG2, S.F2 AS F2, J2.anomaly AS A2, S.ncd_normal AS NCD, ROUND((J.anomaly + J2.anomaly),3) AS AnomalySUM FROM AnomalyQueryStart AS S LEFT JOIN AnomalyJoin AS J USING(IMG1, F1) LEFT JOIN AnomalyJoin AS J2 ON S.IMG2 = J2.IMG1 AND S.F2 = J2.F1;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	res = mysql_use_result(conn);
	

	
	if(GLOB_INIT_FLAG) fprintf(stderr,"\tCreating temporary table %s\n","Collaborative_Result_Temp");
	snprintf(sqlbuffer,BIGBUFFER,"DROP TABLE IF EXISTS Collaborative_Result_Temp;");
	if (mysql_query(conn,sqlbuffer) != 0)
		  mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE Collaborative_Result_Temp AS select IF(MOD(IMG1,%d) = 0, %d, MOD(IMG1,%d)) AS IMG1, IF(MOD(IMG2,%d) = 0, %d, MOD(IMG2,%d)) AS IMG2, SUM(AnomalySUM) As SUM FROM Collaborative_Start_Temp WHERE IMG2 > IMG1 AND AnomalySUM > 0 AND NCD < %f GROUP BY IMG1,IMG2;",MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE,CUTOFF);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	res = mysql_use_result(conn);


}

void getTabledata(MYSQL *connread, char *table, int limit_value, int time)
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
	int image_one;
	int image_two;
	float collaboration_num;
	
	
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
			image_one = atoi(row[0]);
			image_two = atoi(row[1]);
			collaboration_num = atof(row[2]);
			array[image_one][image_two][time] = collaboration_num;
	}
	
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
  {"mod",  required_argument, NULL, 'm'},
  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("NCD Snapshot V.001 - Output data to be graphed.\n");
    printf("Usage: ./collaboration [OPTIONS] \n");
    printf("\t-H, --host=name\t\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-S, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
    printf("\t-f, --file=name\t\t Insted of MySQL - Dumps Insert statements into file: name\n");
	printf("\t-I, --intialize\t\t Show the initialize of the temporary tables.\n");
	printf("\t-C, --cutoff\t\t Change NCD cutoff value. Currently: %f\n",CUTOFF);
	printf("\t-s, --start\t\t Show Collabrative_Start View\n");
    printf("\t-l, --limit\t\t Limit the output of the show command.\n");
	printf("\t-m, --mod\t\t Create table with Modular math inserted into it. Example for 20 images use: -m 20.\n");
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
	int x,y,z, check;
    char sqlbuffer[BIGBUFFER];
  
    //Setup for MySQL Init File
    my_init();
    
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "hh:IC:u:Q:qp:P:S:il:sm:", long_options, &option_index)) != EOF )
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
			CUTOFF = atof(tempstring);
			if(CUTOFF <= 0) { fprintf(stdout,"Bad limit value entered.\n"); exit(1); }
			break;
		case 'm' :
			strncpy(tempstring,optarg,19);
			MODVALUE = atoi(tempstring);
			if(MODVALUE <= 0) { fprintf(stdout,"Bad limit value entered.\n"); exit(1); }
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
	
	FILE *outcsv;
	if( (outcsv= fopen("output.csv", "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n","output.csv"); }
	
	
    //Setup SQL  
	printf("Setting up SQL Connection\n");
    conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
    
	//Setting up main array
	printf("Setting up Array for %d Images and %d iterations.\n", ARRAYSIZE_IMAGES, ARRAYSIZE_TIME);
	for(x =0; x < ARRAYSIZE_IMAGES; x++)
		for(y=0; y < ARRAYSIZE_IMAGES; y++)
			for(z=0; z < ARRAYSIZE_TIME; z++)
				array[x][y][z] = 0;
	
	//Get Data
	printf("Starting to get data.\n");
	for(z = 1; z <= ARRAYSIZE_TIME; z++)
	{
		printf("Getting data for iteration %d of %d.\n",z,ARRAYSIZE_TIME);
		fflush(stdout);
		initTables(conn, z);
		getTabledata(conn, "Collaborative_Result_Temp", limit_value, z);
	}
	
	//Output to file
	for(x =0; x < ARRAYSIZE_IMAGES; x++)
		for(y=0; y < ARRAYSIZE_IMAGES; y++)
		{
			//Print Pair
			check = 0;
			for(z=0; z < ARRAYSIZE_TIME; z++) 
			{
				//Check for anything
				if(array[x][y][z] != 0) check = 1;			
			}
			if(check == 1) 
			{
				fprintf(outcsv,"%d, %d",x,y);
				for(z=0; z < ARRAYSIZE_TIME; z++)
				{
					//print values
					fprintf(outcsv,", %f",array[x][y][z]);
				}
				fprintf(outcsv,"\n");
			}
		}
	
    //Sql End
	fclose(outcsv);
    mysql_close(conn);
    
    return(0);
}


