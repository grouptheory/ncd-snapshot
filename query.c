#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
//#include <mysql/my_sys.h>
#include <getopt.h>
#include "mysql.h"

#define BIGBUFFER 10000
#define sqlbufsize 255

#define db_name "SNAPSHOT"
#define DRIVE_TABLENAME "image_snapshot_table"
#define IMAGE_LIST_TABLENAME "image_list_table"
#define CASE_TABLENAME "case_table"
#define NCD_TABLENAME "NCD_table"
#define QUERY_TABLENAME "query_table"
#define NCD_RESULT "NCD_result"

#ifdef NOSPIN
#define DVERBOSE 1
#else
#define DVERBOSE 0
#endif



void initTables(MYSQL *);
int checkfile(const char *);
void printhelp();
//void showTable(MYSQL *, int, int);
int pickSerial(MYSQL *);
void createtable(MYSQL *, MYSQL *, int, int, int);
int pickQuery(MYSQL *, char *, int, int);

//Flags
int GLOB_INIT_FLAG = 0; //Initialize Tables - Default NO
int GLOBAL_VERBOSE = DVERBOSE;
int GLOBAL_NEWQUERY = 0;
float GLOBAL_BASE = 0;
int GLOBAL_TIME_THRESHOLD = 1000;
int GLOBAL_SIZE_THRESHOLD = 1000;
char GLOBAL_TIME_THRESHOLD_TYPE[100];
int GLOBAL_DELTA_ACCESS = 0;
int GLOBAL_DELTA_MODIFY = 0;
int GLOBAL_DELTA_CREATE = 0;
int GLOBAL_DELTA_SIZE = 0;
int GLOBAL_COMPARE = 0;
int GLOBAL_BESTMATCHING = 0;
char cursor[4]={'/','-','\\','|'}; //For Spinning Cursor




void createtable(MYSQL *connread, MYSQL *connwrite, int querynum, int casenum, int item_num)
{

	char sqlbuffer[BIGBUFFER];
	char second_sqlbuffer[BIGBUFFER];
	MYSQL_RES *res;
	MYSQL_ROW row;
	float ncd = 0;
	short int spot;	
	unsigned long long counter = 0;
	int besteffort = 0;

	snprintf(sqlbuffer, BIGBUFFER,"select DISTINCT a.item, b.item FROM %s AS a JOIN image_snapshot_table AS b JOIN image_list_table As i where a.image IN (select image FROM image_list_table WHERE image_casenumber = %d ) AND b.image IN (select image FROM image_list_table WHERE image_casenumber = %d ) AND NOT a.file_type = 'd' AND NOT b.file_type = 'd' AND (0 ",DRIVE_TABLENAME, casenum, casenum);
	

	if(GLOBAL_DELTA_ACCESS == 1)
		{
			snprintf(second_sqlbuffer,BIGBUFFER," + (ABS(TIMESTAMPDIFF(%s,a.timestamp_access, b.timestamp_access)) < %d) ", GLOBAL_TIME_THRESHOLD_TYPE, GLOBAL_TIME_THRESHOLD);
			strncat(sqlbuffer, second_sqlbuffer, BIGBUFFER);
			besteffort++;
		}
	if(GLOBAL_DELTA_MODIFY == 1)
		{
			snprintf(second_sqlbuffer,BIGBUFFER," + (ABS(TIMESTAMPDIFF(%s,a.timestamp_modify, b.timestamp_modify)) < %d)", GLOBAL_TIME_THRESHOLD_TYPE, GLOBAL_TIME_THRESHOLD);
			strncat(sqlbuffer, second_sqlbuffer, BIGBUFFER);
			besteffort++;
		}
	if(GLOBAL_DELTA_CREATE == 1)
		{
			snprintf(second_sqlbuffer,BIGBUFFER," + (ABS(TIMESTAMPDIFF(%s,a.timestamp_create, b.timestamp_create)) < %d)", GLOBAL_TIME_THRESHOLD_TYPE, GLOBAL_TIME_THRESHOLD);
			strncat(sqlbuffer, second_sqlbuffer, BIGBUFFER);
			besteffort++;
		}
	if(GLOBAL_DELTA_SIZE == 1)
		{
			snprintf(second_sqlbuffer,BIGBUFFER," + ( ABS(CAST(a.file_size - b.file_size AS SIGNED)) < %d)", GLOBAL_SIZE_THRESHOLD);
			strncat(sqlbuffer, second_sqlbuffer, BIGBUFFER);
			besteffort++;
		}
	
	if(GLOBAL_BESTMATCHING > 0) besteffort = GLOBAL_BESTMATCHING;
	
	snprintf(second_sqlbuffer,BIGBUFFER," ) >= %d ", besteffort);
	strncat(sqlbuffer, second_sqlbuffer, BIGBUFFER);
	
	if(GLOBAL_COMPARE == 1)
		{
			snprintf(second_sqlbuffer,BIGBUFFER," AND a.item = %d",item_num);
			strncat(sqlbuffer, second_sqlbuffer, BIGBUFFER);
		}

	 
	fprintf(stderr,"Creating new query and inserting it into the NCD Table.\n");
   // if(GLOBAL_VERBOSE) printf("Verbose Output:%s\n",sqlbuffer);
	//printf("Query %s\n",sqlbuffer);	    

	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);

	res = mysql_use_result(connread);

	while( (row = mysql_fetch_row(res)) != NULL)
	{
		snprintf(second_sqlbuffer, BIGBUFFER,"INSERT INTO NCD_table (querynumber, file_one, file_two) VALUES (\"%d\",\"%s\",\"%s\");", querynum,row[0], row[1]);
	    counter++;
	    if (mysql_query(connwrite,second_sqlbuffer) != 0)
		mysql_print_error(connwrite);
	//printf("Query %s\n",second_sqlbuffer);	    
	    if(!GLOBAL_VERBOSE) 
	    {
	      printf("%c\b", cursor[spot]); 
	      fflush(stdout);
	      spot = (spot+1) % 4;
	    }    
	 }
	 
	 fprintf(stderr,"Finished: Query [%d] contains %u new records.\n",querynum,counter);

}


void initTables(MYSQL *conn)
{
	if(conn == NULL) exit(1);
	
	char sqlbuffer[BIGBUFFER];
	
	fprintf(stderr,"Intializing Tables in Database\n");
	fprintf(stderr,"\tDropping table %s\n",NCD_TABLENAME);
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;", NCD_TABLENAME);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n",NCD_TABLENAME);
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_NCDTABLE);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
		
	fprintf(stderr,"\tDropping table %s\n",NCD_RESULT);
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;", NCD_RESULT);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n",NCD_RESULT);
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_NCDRESULT_TABLE);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
		
	fprintf(stderr,"\tDropping table %s\n",QUERY_TABLENAME);
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;",QUERY_TABLENAME);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n",QUERY_TABLENAME);
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_QUERYTABLE);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

}

int pickQuery(MYSQL *conn, char *query_number, int case_num, int file_num)
{
   	if(conn == NULL) exit(1);

	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlbuffer[BIGBUFFER+2];
	char second_sqlbuffer[BIGBUFFER+2];
	char tempstring[20];
	char query_name[sqlbufsize];
	int query_num = 0;
	int check = 0;
	my_ulonglong num_rows;
	int replace = 0;
	
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
				query_num = query_num + 1;
			}
		else {query_num = 1; check = 1;} 
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
			printf("Wiping old values from previous query [%d]\n",query_num);
			snprintf(second_sqlbuffer, BIGBUFFER, "DELETE FROM %s WHERE querynumber = %d;", NCD_TABLENAME, query_num);
			replace = 1;
			if (mysql_query(conn,second_sqlbuffer) != 0)
				mysql_print_error(conn);
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
				printf("Wiping old values from previous query [%d]\n",query_num);
				while( (row = mysql_fetch_row(res)) != NULL);
				snprintf(second_sqlbuffer, BIGBUFFER, "DELETE FROM %s WHERE querynumber = %d;", NCD_TABLENAME, query_num);
				replace = 1;
				if (mysql_query(conn,second_sqlbuffer) != 0)
					mysql_print_error(conn);
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
		else if ( !strncasecmp(tempstring, "N", 1) )
		{
			snprintf(sqlbuffer, BIGBUFFER,"select MAX(querynumber) FROM %s;",NCD_TABLENAME);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
			res = mysql_use_result(conn);
			if( ((row = mysql_fetch_row(res)) != NULL) && row[0] != NULL)
				{
					query_num = atoi(row[0]);
					check = 1;
					query_num = query_num + 1;
				}
			else {query_num = 1; check = 1;} 
			while( (row = mysql_fetch_row(res)) != NULL);
		}
		else if (!strncasecmp(tempstring, "?", 1) )
		{
			printf("Help:\nEnter number of a query. \nEnter [L] for List of current querys. \nEnter [N] to create a new query\n"); 		
		}	
	}//While

	if(replace)
	{
		snprintf(second_sqlbuffer, BIGBUFFER, "DELETE FROM %s WHERE querynumber = %d;", QUERY_TABLENAME, query_num);
		if (mysql_query(conn,second_sqlbuffer) != 0)
					mysql_print_error(conn);
	}
	
	snprintf(second_sqlbuffer, BIGBUFFER,"INSERT INTO %s (querynumber, casenumber, delta_time_threshold, delta_size_threshold, delta_unit, delta_modify, delta_access, delta_create, delta_size, compare_file, best_effort) VALUES (\"%d\",\"%d\",\"%d\",\"%d\",\"%s\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\",\"%d\");", QUERY_TABLENAME, query_num, case_num, GLOBAL_TIME_THRESHOLD, GLOBAL_SIZE_THRESHOLD, GLOBAL_TIME_THRESHOLD_TYPE, GLOBAL_DELTA_MODIFY, GLOBAL_DELTA_ACCESS, GLOBAL_DELTA_CREATE, GLOBAL_DELTA_SIZE, file_num, GLOBAL_BESTMATCHING);

	if (mysql_query(conn,second_sqlbuffer) != 0)
		mysql_print_error(conn);
				
	return query_num;
}

int pickCase(MYSQL *conn, char *case_number)
{
   	if(conn == NULL) exit(1);

	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlbuffer[BIGBUFFER];
	char tempstring[20];
	char case_name[sqlbufsize];
	int case_num = 0;
	int check = 0;
	my_ulonglong num_rows;
	
	//Check provided number
	if(case_number != NULL)
	if( (case_num = atoi(case_number)) > 0 )
	{//perform Check
		snprintf(sqlbuffer, BIGBUFFER,"select case_number FROM %s WHERE case_number = %d",CASE_TABLENAME, case_num);
		if (mysql_query(conn,sqlbuffer) != 0)
			mysql_print_error(conn);
		res = mysql_store_result(conn);
		num_rows = mysql_num_rows(res);
		if(num_rows == 1) check = 1;
		else printf("Error with case value provided\n");
		while( (row = mysql_fetch_row(res)) != NULL); //goto end
	}
	
	while(check == 0)
	{
		printf("\nPlease enter case number [L][?]: ");
		fgets(tempstring,18,stdin);
		case_num = atoi(tempstring);
		if(case_num > 0) //Number
		{	//perform check
			snprintf(sqlbuffer, BIGBUFFER,"select case_number FROM %s WHERE case_number = %d",CASE_TABLENAME,case_num);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
			res = mysql_store_result(conn);
			num_rows = mysql_num_rows(res);
			if(num_rows == 1) check = 1;
			else printf("Error with case value [%d] provided.\n",case_num);
			while( (row = mysql_fetch_row(res)) != NULL); //goto end			
		}
		else if ( !strncasecmp(tempstring, "L", 1) )
		{
			//List Current Cases
			snprintf(sqlbuffer, BIGBUFFER,"select * FROM %s",CASE_TABLENAME);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
			res = mysql_store_result(conn);
			printf("Case #\tCase Name\n");
			while( (row = mysql_fetch_row(res)) != NULL)
				{
					printf("%-7s\t%-30s\n", row[0],row[1]);
				}
		}
		else if (!strncasecmp(tempstring, "?", 1) )
		{
			printf("Help:\nEnter number of a case. \nEnter [L] for List of current cases. \n"); 		
		}	
	}//While
	
	return case_num;
}

int pickFile(MYSQL *connread, int serial, char *filename)
{
   	if(connread == NULL) exit(1);

	MYSQL_RES *res;
	MYSQL_ROW row;
	my_ulonglong num_rows;
	
	char sqlbuffer[BIGBUFFER];
	char tempstring[20];
	int item_num = 0;
	int checkB;
	int checkS;
	snprintf(sqlbuffer, BIGBUFFER,"select a.item, CONCAT(a.directory,'/', a.filename) FROM image_snapshot_table AS a where a.image IN ( select image FROM image_list_table WHERE image_casenumber = %d ) AND a.filename = \"%s\"",serial,filename);
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_store_result(connread);
	num_rows = mysql_num_rows(res);
	
	if(num_rows == 1)
	{
		row = mysql_fetch_row(res);
		item_num = atoi(row[0]);
		printf("Found %s in image %d. (Item number %d)\n",filename,serial, item_num);
		while( (row = mysql_fetch_row(res)) != NULL); //goto end
		return item_num;
	}
	
	if(num_rows > 1) printf("Found too many items with that file name. [%d] You must pick from a list.\n",num_rows);
	if(num_rows == 0) printf("Could not find that filename [%s]. You must pick from a list. %d\n", filename, num_rows);
	while( (row = mysql_fetch_row(res)) != NULL); //goto end
	
	fprintf(stderr,"Item\tFileName \n");
	snprintf(sqlbuffer, BIGBUFFER,"select a.item, CONCAT(a.directory,'/', a.filename) FROM image_snapshot_table AS a where a.image IN ( select image FROM image_list_table WHERE image_casenumber = %d );",serial);

	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);

	while( (row = mysql_fetch_row(res)) != NULL)
	{
	    printf("%-7s\t%30s\n", row[0],row[1]);
	}
	printf("\nYou must pick a file by its item number:\n");
	fgets(tempstring,18,stdin);
	item_num = atoi(tempstring);
	
	//select MAX(image) FROM image_list_table; Get Max Value
	snprintf(sqlbuffer, BIGBUFFER,"select MAX(a.item), MIN(a.item) FROM image_snapshot_table AS a where a.image IN ( select image FROM image_list_table WHERE image_casenumber = %d );", serial);
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);
	if( (row = mysql_fetch_row(res)) == NULL) return 0;
	checkB = atoi(row[0]);
	checkS = atoi(row[1]);
	while( (row = mysql_fetch_row(res)) != NULL); //goto end
	
	if( (item_num < checkS) || (item_num > checkB)) { printf("Item number is not in range.\n"); return 0; }
	else return item_num;
	
}


static const char *groups[] = {"client", 0};

struct option long_options[] =
{
  {"host", required_argument, NULL, 'H'},
  {"user", required_argument, NULL, 'u'},
  {"password", required_argument, NULL, 'p'},
  {"port", required_argument, NULL, 'P'},
  {"socket", required_argument, NULL, 'O'},
  {"case-number", required_argument, NULL, 'K'},
  {"query-number", required_argument, NULL, 'Q'},
  {"query-new", no_argument, NULL, 'q'},
  {"help", no_argument, NULL, 'h'},
  {"intialize", no_argument, NULL, 'I'},
  {"compare", required_argument, NULL, 'R'},
  {"delta-time", required_argument, NULL, 'D'},
  {"delta-type", required_argument, NULL, 'U'},
  {"delta-modify", no_argument, NULL, 'M'},
  {"delta-access", no_argument, NULL, 'A'},
  {"delta-create", no_argument, NULL, 'C'},
  {"delta-size-threshold", required_argument, NULL, 'T'},
  {"delta-size", no_argument, NULL, 'S'},
  {"besteffort", required_argument, NULL, 'b'},

  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("NCD Snapshot Suite V.001 - This program filters files to perform NCD operations on.\n");
    printf("Defaults may be stored in your home directory in ./.snapshot.cnf\n");
    printf("Usage: ./query [OPTIONS] --serial # \n");

    printf("\t-H, --host=name\t\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-O, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
	printf("\t-v, --verbose\t\t Provide extra output.\n");
    printf("\t-I, --intialize\t\t Initialize new tables (Warning: wipes/drops old).\n");
	printf("\t-K, --case-number\t Case Number\n");
	printf("\t-Q, --query-number\t Query Number\n");
	printf("\t-q, --query-new\t\t Do not prompt about query, just create a new one.\n");
    printf("\t\t\t\t\t File Options\n");
    printf("\t-R, --compare\t\t\t Compare only with a specific file in snapshot. EX: -R filename.ext\n");
	printf("\t-D, --delta-time\t\t Minimal Threshold used for MAC Timestamps. Default: %d\n",GLOBAL_TIME_THRESHOLD);
	printf("\t-U, --delta-type\t\t Threshold Unit of time. Default: %s\n",GLOBAL_TIME_THRESHOLD_TYPE);
	printf("\t\t\t\t\t [FRAC_SECOND, SECOND, MINUTE, HOUR, DAY, WEEK, MONTH, QUARTER, YEAR]\n");
    printf("\t-M, --delta-modify\t\t Match files using the delta of the modified timestamp.\n");    
	printf("\t-A, --delta-access\t\t Match files using the delta of last accessed timestamp.\n");
	printf("\t-C, --delta-create\t\t Match files using the delta of the created timestamp.\n");
	printf("\t-T, --delta-size-threshold\t Minimal Threshold used for file size matching. (Bytes) Default: %d\n",GLOBAL_SIZE_THRESHOLD);
	printf("\t-S, --delta-size\t\t Match files using the delta of two different file sizes.\n");
	printf("\t-b, --besteffort\t\t Number of options to match. (Best Effort) The default is to match all options.\n");
    printf("\t-h, --help\t\t\t This help page.\n");
  
}
#define PASS_SIZE 100
int main(int argc, char *argv[] )
{
  
    struct dirDFS *directory;
    char *host_name = NULL;
    char *user_name = NULL;
    char *password = NULL;
	char cfilename[PATH_MAX+1];
    char tempstring[20];
    char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
    unsigned int port_num = 0;
    char *socket_name = NULL;
    int input,option_index,key;
    char scandir[PATH_MAX+1];
    FILE *fileoutput = NULL;
    char OUTPUT_PATH[PATH_MAX+1];
    MYSQL *connread = NULL;
    MYSQL *connwrite = NULL;
	char *case_number = NULL;
	char *query_number = NULL;
    char *investigator = NULL;
    char *case_name = NULL;
    char *image_name = NULL;
    int case_num = 0;
	int query_num = 0;
    int limit_value = 0;
    int krepeat = 0;
	int item_num;
	sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"MINUTE");
    
    
    //Setup For OpenSSL Hash
    OpenSSL_add_all_digests();
    
    //Setup for MySQL Init File
    my_init();
    
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "hH:u:R:Q:qK:D:T:p:P:O:Ic:MACSvU:b:", long_options, &option_index)) != EOF )
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
			case 'R' :
			 GLOBAL_COMPARE  = 1;
			  strncpy(cfilename,optarg,PATH_MAX);
			  break;	  
			case 'Q' :
			  query_number = optarg;
			  break;
			case 'q' :
			  GLOBAL_NEWQUERY = 1;
			  break;
			case 'K' :
			  case_number = optarg;
			  break;
			case 'D' :
			  strncpy(tempstring,optarg,19);
			  GLOBAL_TIME_THRESHOLD = atoi(tempstring);
			  if(GLOBAL_TIME_THRESHOLD <= 0)
			  {
				fprintf(stderr,"Error reading Delta Timestamp threshold. Using default.\n"); 
				GLOBAL_TIME_THRESHOLD = 1000;
			  }
			  break;
			case 'T' :
			  strncpy(tempstring,optarg,19);
			  GLOBAL_SIZE_THRESHOLD = atoi(tempstring);
			  if(GLOBAL_SIZE_THRESHOLD <= 0)
			  {
				fprintf(stderr,"Error reading Delta file size threshold. Using default.\n"); 
				GLOBAL_SIZE_THRESHOLD = 1000;
			  }
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
			case 'O' :
			  socket_name = optarg;  
			  break;
			case 'I' :
			  GLOB_INIT_FLAG = 1;
			  break;
			case 'M' :
				GLOBAL_DELTA_MODIFY = 1;
			break;
			case 'A' :
				GLOBAL_DELTA_ACCESS = 1;
			break;
			case 'C' :
				GLOBAL_DELTA_CREATE = 1;
			break;
			case 'S' :
				GLOBAL_DELTA_SIZE = 1;
			break;
			case 'v' :
				GLOBAL_VERBOSE = 1;
			break;
			case 'U':
				//Type
				strncpy(tempstring, optarg, 19);
				if(!strncasecmp(tempstring, "FRAC_SECOND", 9)) {	sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"FRAC_SECOND");}
				else if(!strncasecmp(tempstring, "SECOND", 6)) {	sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"SECOND");}
				else if(!strncasecmp(tempstring, "MINUTE", 6)) {	sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"MINUTE");}
				else if(!strncasecmp(tempstring, "HOUR", 4)) {		sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"HOUR");}
				else if(!strncasecmp(tempstring, "DAY", 3)) {		sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"DAY");}
				else if(!strncasecmp(tempstring, "WEEK", 4)) {		sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"WEEK");}
				else if(!strncasecmp(tempstring, "MONTH", 5)) {		sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"MONTH");}
				else if(!strncasecmp(tempstring, "QUARTER", 7)) {	sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"QUARTER");}
				else if(!strncasecmp(tempstring, "YEAR", 4)) {		sprintf(GLOBAL_TIME_THRESHOLD_TYPE,"YEAR");}
				else {printf("Invalid Options. Using default. Valid options are FRAC_SECOND, SECOND, MINUTE, HOUR, DAY, WEEK, MONTH, QUARTER, and YEAR.\n");}
			break;
			case 'b' :
			  strncpy(tempstring,optarg,19);
			  GLOBAL_BESTMATCHING = atoi(tempstring);
			  if(GLOBAL_BESTMATCHING <= 0)
			  {
				fprintf(stderr,"Error reading best effor matching value Using default.\n"); 
				GLOBAL_BESTMATCHING = 0;
			  }
			  break;
		  
    }//switch      
     
    }//while
 

    //Move to end of Argument List
    argc -= optind; 
    argv += optind;
    
    //Used CWD if nothing else specified
	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
    

     //Setup SQL  
     connread = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
     connwrite = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);

     if(GLOB_INIT_FLAG == 1)
     {
	initTables(connwrite);
     }  
     else 
     {
		case_num = pickCase(connread, case_number);
		if(GLOBAL_COMPARE == 1)
		 {
			item_num = pickFile(connread, case_num, cfilename);
		 }
		 query_num = pickQuery(connread, query_number, case_num, item_num);
		 createtable(connread, connwrite, query_num, case_num, item_num);
     }

     //Sql End
     if (mysql_query(connwrite,"COMMIT;") != 0)
			mysql_print_error(connwrite);
      mysql_close(connread);
      mysql_close(connwrite);

    
    return(0);
}


