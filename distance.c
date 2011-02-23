#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
//#include <mysql/my_sys.h>
#include <getopt.h>
#include <pthread.h>
#include <dlfcn.h>
#include "mysql.h"

#define DEFAULT_DISTANCE_LIB "/usr/local/lib/libncd.so.1.0"


#define BIGBUFFER 10000
#define PASS_SIZE 100
#define db_name "SNAPSHOT"
#define DRIVE_TABLENAME "image_snapshot_table"
#define IMAGE_LIST_TABLENAME "image_list_table"
#define NCD_TABLENAME "NCD_table"
#define QUERY_TABLENAME "query_table"
#define NCD_RESULT_TABLENAME "NCD_result"

#define sqlbufsize 255
#define DEFAULT_CHUNK 16000
#define DEFAULT_SHOW 10
#define DEFAULT_TINYCHUNK 8000

#ifdef NOSPIN
#define DVERBOSE 1
#else
#define DVERBOSE 0
#endif


void initTables(MYSQL *);
void ncdsnapshot(MYSQL *, int);
int checkfile(const char *);
void printhelp();
void showTableQuery(MYSQL *, int, int);
int pickSerial(MYSQL *);
void * ncdThread(void *);

//ProtoTypes for dynamic linking
typedef void (*distanceFunction)(char *, char *, float *, float *);
typedef void (*setoptFunction)(char *, void *);
distanceFunction distance;
setoptFunction setopt;
void *distancelib;

//Flags
int GLOB_INIT_FLAG = 0; //Initialize Tables - Default NO
int GLOBAL_VERBOSE = DVERBOSE;
int GLOBAL_SHOWMATCHING = 0;
float GLOBAL_BASE = 0;
int GLOBAL_SHOWOUT = 0;
int GLOBAL_COMPARE = 0;
int GLOBAL_NEWQUERY = 0;
int GLOBAL_THREADS = 4;
int GLOBAL_SKIPDEL = 0;


char cursor[4]={'/','-','\\','|'}; //For Spinning Cursor

//MySQL Login Information
char *host_name = NULL;
char *user_name = NULL;
char *password = NULL;
char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
unsigned int port_num = 0;
char *socket_name = NULL;
	
	
struct storage_struct {
	long int min;
	long int max;
	pthread_t TID; //thread ID
};



void ncdsnapshot(MYSQL *connread, int query_num)
{

	char sqlbuffer[BIGBUFFER+2];
	MYSQL_RES *res;
	MYSQL_ROW row;
	long int min, max, total, start, end, modulus;
	int thread_count = GLOBAL_THREADS;
	int c;
	struct storage_struct thread_storage[GLOBAL_THREADS]; //create global container
	pthread_attr_t attr;
	//get default attributes 
	pthread_attr_init(&attr);
	//Must be set to wait for Threads to finish
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
	
	//Removal
	if(GLOBAL_SKIPDEL == 0)
	{
	snprintf(sqlbuffer, BIGBUFFER,"DELETE QUICK IGNORE FROM NCD_result USING NCD_result, NCD_table WHERE (NCD_result.ncd_key = NCD_table.ncd_key) AND (NCD_table.querynumber = %d);",query_num);
	fprintf(stderr,"Removing any previous NCD values related to query [%d].\n", query_num);
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	}
	
	//Get Information  (min,max,total)
	snprintf(sqlbuffer, BIGBUFFER,"SELECT MIN(ncd_key), MAX(ncd_key), COUNT(ncd_key) FROM NCD_table WHERE querynumber = %d ;",query_num);
	fprintf(stderr,"Getting information about query.\n");
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);
	if( (row = mysql_fetch_row(res)) == NULL ) { fprintf(stderr,"Error getting data.\n"); exit(1); }
	else
	{
		min = atol(row[0]);
		max = atol(row[1]);
		total = atol(row[2]);
	}
	if(total == 0) { fprintf(stderr,"Nothing to compute NCD on!\n"); exit(1); }
	while( (row = mysql_fetch_row(res)) != NULL);
	


	//Fill in Min/Max
	for(c = 0; c < GLOBAL_THREADS; c++)
	{
		if(c == 0) 
		{ 
			modulus = total / GLOBAL_THREADS; 
			if(modulus == 0)
			{
				//More threats then total number? Ouch - only open one thread
				thread_storage[c].min = min;
				thread_storage[c].max = max;
				thread_count = 1;
				break;
			}
			start = min;
			end = min + modulus - 1;
		} //c== 0
		else if ( c == (GLOBAL_THREADS -1) )
		{
			start = end + 1;
			end = max;
		}
		else
		{
			start = end + 1;
			end = start + modulus -1;
		}
		
		thread_storage[c].min = start;
		thread_storage[c].max = end;
	}
	//Fill in min/max
	
	//Start Threads
	for(c = 0; c < thread_count; c++)
	{
		pthread_create(&thread_storage[c].TID, &attr, ncdThread, (void *) &thread_storage[c]);
		printf("Starting thread[%lu] %ld of %ld to compute NCD Values\n", thread_storage[c].TID, c+1, thread_count);
	}	
	
	//Wait for Threads to finish
	for (c=0; c < thread_count; c++)
	{
		pthread_join(thread_storage[c].TID, NULL);
	}
	
	printf("Running a COMMIT\n");
	if (mysql_query(connread,"COMMIT;") != 0)
		mysql_print_error(connread);

	 fprintf(stderr,"NCD Values have been added to the NCD Result Table.\n");
	 
}//ncdsnapshot

void * ncdThread(void *parm)
{
	//creat data structure
	struct storage_struct *data;
	//Parm is a void of any structure, form parm into actual data structure 
	data = (struct storage_struct *)parm;
	char sqlbuffer[BIGBUFFER+2];
	char second_sqlbuffer[BIGBUFFER+2];
	char third_sqlbuffer[BIGBUFFER];
	char file_one[PATH_MAX];
	char file_two[PATH_MAX];
	MYSQL *connread = NULL;
	MYSQL *connwrite = NULL;
	MYSQL_RES *res;
	MYSQL_ROW row;
	float ncd = 0;
	float dncd = 0;
	int insertcount = 0;
	
	connread = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	if(connread == NULL) { fprintf(stderr,"Error opening MySQL Connection.\n"); exit(1); }
	connwrite = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	if(connwrite == NULL) { fprintf(stderr,"Error opening MySQL Connection.\n"); exit(1); }
	
	snprintf(sqlbuffer, BIGBUFFER,"	select n.ncd_key, CONCAT(a.directory,'/', a.filename), CONCAT(b.directory, '/', b.filename) FROM image_snapshot_table AS a JOIN image_snapshot_table AS b JOIN NCD_table AS n ON n.file_one = a.item AND n.file_two = b.item AND n.ncd_key >= %ld AND n.ncd_key <= %ld;", data->min, data->max);

	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);
	
	 
	while( (row = mysql_fetch_row(res)) != NULL)
	{
		ncd = 0; dncd =0;
		distance(row[1], row[2], &ncd, &dncd);
	    if (ncd == -999) continue; //skip if we had an error
		
		if(insertcount == 0) { snprintf(second_sqlbuffer, BIGBUFFER, "INSERT INTO %s VALUES (\"%s\", \"%f\", \"%f\") ", NCD_RESULT_TABLENAME, row[0], ncd, dncd); insertcount++;}
		else if (insertcount > 100)
		{
			snprintf(third_sqlbuffer, BIGBUFFER, ", (\"%s\", \"%f\", \"%f\");", row[0], ncd, dncd);
			strncat(second_sqlbuffer, third_sqlbuffer, BIGBUFFER);
			if (mysql_query(connwrite,second_sqlbuffer) != 0)
			mysql_print_error(connwrite);	
			insertcount = 0;
		
		}
		else
		{
			snprintf(third_sqlbuffer, BIGBUFFER, ", (\"%s\", \"%f\", \"%f\") ", row[0], ncd, dncd);
			strncat(second_sqlbuffer, third_sqlbuffer, BIGBUFFER);
			insertcount++;
		}
	
	 }
	
	if (insertcount > 0)
	{
		strncat(second_sqlbuffer, ";", BIGBUFFER);
		if (mysql_query(connwrite,second_sqlbuffer) != 0)
		mysql_print_error(connwrite);		
	}
	
	mysql_close(connread);
	mysql_close(connwrite);
	
	return((void *)0); //returns pointer NULL of no datatype
}//end of ncdThread

void showTableQuery(MYSQL *connread, int query_num, int limit_value)
{
      	if(connread == NULL) exit(1);

	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlbuffer[BIGBUFFER];
	char sqlstartbuffer[BIGBUFFER];
    fprintf(stderr,"Image\t%-50s\t\tImage\t%-50s\tNCD\tDouble NCD\n","File Number One", "File Number Two");
	
	snprintf(sqlstartbuffer, BIGBUFFER,"select a.image,CONCAT(a.directory,'/', a.filename), b.image, CONCAT(b.directory, '/', b.filename), ncd_normal, ncd_double FROM NCD_table AS n JOIN NCD_result AS r JOIN image_snapshot_table AS a JOIN image_snapshot_table AS b WHERE n.ncd_key = r.ncd_key AND n.file_one = a.item AND n.file_two = b.item AND n.querynumber = %d AND NOT file_one = file_two AND NOT a.filename = b.filename ORDER BY NCD_normal,NCD_double ",query_num);
	
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
  {"double", no_argument, NULL, 'D'},
  {"tinychunk", required_argument, NULL, 'T'},
  {"threads", required_argument, NULL, 'A'},
  {"nodelete", no_argument, NULL, 'N'},
  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("NCD SQL V.001 - Calculates NCD values of files in within the Query NCD table.\n");
    printf("Defaults may be stored in your home directory in ./.snapshot.cnf\n");
    printf("Usage: ./ncd [OPTIONS] --serial # \n");

    printf("\t-H, --host=name\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-S, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
    printf("\t-f, --file=name\t Insted of MySQL - Dumps Insert statements into file: name\n");
    printf("\t-Q, --query-num\t Query number to use.\n");
	printf("\t-q, --query\t	Use the last query created.\n");
    printf("\t-w, --show\t Show Table when completed\n");
    printf("\t-l, --limit\t Limit the output of the show command.\n");
	printf("\t-A, --threads\t Change number of threAds to use.\n");
    printf("\t-N, --nodelete\t Skips the removal of previous Values.\n");
	printf("\t-L, --library\t Change distance library.\n");
	printf("\t\t\t NCD Options\n");
    printf("\t-c, --chunk\t The value of bytes that we check,\n\t\t\t Default is currently set to: %d\n",DEFAULT_CHUNK);
    printf("\t-o, --random\t Use a random offset to compare.\n");
    printf("\t-k, --repeat\t Optional argument to be used with --random option.\n\t\t\t Repeats operation K times. EX: -k 2.\n");
    printf("\t-O, --offset\t Use a specifc offset in file (Bytes). EX: -O 256\n");
	printf("\t-D, --double\t Complete a double NCD of each selection\n");
	printf("\t-T, --tinychunk\t Change the default size of the small chunk size.\n\t\t\t Default is currently set to: %d\n", DEFAULT_TINYCHUNK);
	
    
    printf("\t-h, --help\t This help page.\n");
  
}

int openLib(char *string_lib)
{
	char *err;
	//dlclose(distancelib);
	dlerror();
	
	distancelib = dlopen(string_lib, RTLD_NOW);
	if(!distancelib)
		{
			printf("Failed to open %d: %s\n",string_lib, dlerror());
			return -1;
		}
	dlerror();
	distance = (distanceFunction) dlsym(distancelib,"distanceFunction");
	err=dlerror();
	if(err) 
		{
			printf("Failed to locate distanceFunction: %s\n",dlerror());
			return -1;
		}
		
	setopt = (setoptFunction) dlsym(distancelib,"setoption");
	err=dlerror();
	if(err) 
		{
			printf("Failed to locate setoption: %s\n",dlerror());
			return -1;
		}
	return 1;
}

int main(int argc, char *argv[] )
{
  
    char tempstring[PATH_MAX+1];
    int input,option_index,key;
    FILE *fileoutput = NULL;
    MYSQL *connread = NULL;
    MYSQL *connwrite = NULL;
	char *query_number = NULL;
	int query_num = 0;
    int limit_value = 0;
    int temp_int = 0;
	int item_num;
    int libopen = -1;
    ssize_t offset = 0;

    //Setup for MySQL Init File
    my_init();
    
	libopen = openLib(DEFAULT_DISTANCE_LIB);
	if(libopen == - 1) printf("Warning: Failed to open default distance library: %s\n", DEFAULT_DISTANCE_LIB);
	
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "hH:u:Q:qp:P:S:Dc:O:owl:T:k:A:NL:", long_options, &option_index)) != EOF )
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
			case 'N' :
			  GLOBAL_SKIPDEL = 1;
			  break;		
			case 'w' :
			  GLOBAL_SHOWOUT = 1;
			break;
			case 'l' :
			  strncpy(tempstring,optarg,19);
			 limit_value = atoi(tempstring);
			  if(limit_value < 0) { fprintf(stdout,"Bad limit value entered.\n"); exit(1); }
			break;			  
			case 'A' :
			  strncpy(tempstring,optarg,19);
			  GLOBAL_THREADS = atoi(tempstring);
			  if(GLOBAL_THREADS < 1) { fprintf(stdout,"You need at least 1 thread! Setting to default.\n"); GLOBAL_THREADS = 4; }
			break;
			case 'L' :
			  strncpy(tempstring,optarg,PATH_MAX);
			  openLib(tempstring);
			break;		
			case 'c' :
			  strncpy(tempstring,optarg,19);
			  temp_int = atoi(tempstring);
			  if(libopen > 0) setopt("CHUNK_SIZE", (void *)temp_int);
			  if(temp_int < 8000) { fprintf(stdout,"Bad value entered.\n"); exit(1); }
			break;

			case 'O' :
			  strncpy(tempstring,optarg,19);
			  temp_int = atoi(tempstring);
			  if(libopen > 0) setopt("OFFSET", (void *) temp_int);
			  if(temp_int < 0) { fprintf(stdout,"Bad offset value entered.\n"); exit(1); }
			break;
			case 'D' :
				if(libopen > 0) setopt("DOUBLE", (void *)1);
			  break;
			case 'o' :
			  if(libopen > 0) setopt("RANDOM_OFFSET", (void *)1);
			break;
			case 'T' :
				strncpy(tempstring,optarg,19);
				temp_int = atoi(tempstring);
				if(libopen > 0) setopt("TINY_CHUNK_SIZE", (void *)temp_int);
				if(temp_int < 1000) { fprintf(stdout,"Bad tiny chunk value entered.\n"); exit(1); }
			break;
			case 'k' :
			  strncpy(tempstring,optarg,19);
			  temp_int = atoi(tempstring);
			  if(temp_int < 0) { fprintf(stdout,"Bad value entered.\n"); exit(1); }
			  if(libopen > 0) setopt("RANDOM_K", (void *) temp_int);
			break;
				  
		}//switch      
     
    }//while
    
    //Move to end of Argument List
    argc -= optind; 
    argv += optind;
	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
    
	if(libopen < 0) { printf("Failed to open a library after going over all the options. Must fail. So sorry...\n"); exit(1); }
	
	connread = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);

	query_num = pickQuery(connread, query_number);

	if(query_num > 0)
	{
		ncdsnapshot(connread, query_num);
		if(GLOBAL_SHOWOUT == 1)
		{
			showTableQuery(connread, query_num, limit_value);
		}
	}

	mysql_close(connread);
    dlclose(distancelib);
    return(0);
}


