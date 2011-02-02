#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
#include <getopt.h>
#include "mysql.h"
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>

#define BIGBUFFER 10000
#define sqlbufsize 255
#define db_name "SNAPSHOT"
#define BUFFER 10000

int checkfile(const char *);
void printhelp();

//Flags
int GLOBAL_VERBOSE = 0;
char cursor[4]={'/','-','\\','|'}; //For Spinning Cursor

int checkfile(const char *file)
{
	struct stat buf;
	char fullname[PATH_MAX+1];
	char currentdir[PATH_MAX+1];
	if( getcwd(currentdir,PATH_MAX) == NULL)
		{fprintf(stderr,"Failed to get Current Working directory or directory from command line.\n"); exit(1);}

	if(*file != '/')
	{
		snprintf(fullname,PATH_MAX,"%s/%s",currentdir,file);
	}
	if((stat(file, &buf)) < 0) return 0;
	else if(S_ISREG(buf.st_mode)) return 1;
	else if(S_ISDIR(buf.st_mode)) return 2;
	
	else return 0;	
}

int getValues(char *readline, char *values, int size)
{
	char *sub_ptr = NULL;
	char *clear_ptr;
	int count = 0;
	char tempstring[BUFFER];
	snprintf(values,BUFFER,"(");
	char *line = readline;
	
	while (( sub_ptr = strtok(line,",")) != NULL ) 
	{
		if(count >= size) { fprintf(stderr,"Over flow of values - cutting off"); break; }
		line = NULL;
		clear_ptr = sub_ptr;
		//dirty filter
		while( *clear_ptr != '\0' ) { if(*clear_ptr == '\n' || *clear_ptr == '\r') *clear_ptr = '\0'; clear_ptr++;}
		while( *sub_ptr == ' ' && sub_ptr != '\0' ) sub_ptr++;
		if(count == 0) snprintf(tempstring,BUFFER,"%s\"%s\"",values, sub_ptr);
		else snprintf(tempstring,BUFFER,"%s, \"%s\"",values, sub_ptr);
		strncpy(values, tempstring, BUFFER);
		count++;
	}
	
	while(count < size)
	{
		if(count == 0) snprintf(tempstring,BUFFER,"%s\"\"",values);
		else snprintf(tempstring,BUFFER,"%s, \"\"",values);
		strncpy(values, tempstring, BUFFER);
		count++;
	}
	
	snprintf(tempstring,BUFFER,"%s)",values,strtok(sub_ptr,"\n"));
	strncpy(values, tempstring, BUFFER);
	

	return 0;
}//getValues

void initTables(MYSQL *conn)
{
	if(conn == NULL) exit(1);
	
	char sqlbuffer[BIGBUFFER];
	
	fprintf(stderr,"\tDropping table %s\n","AnomalyCurve");
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;", "AnomalyCurve");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

	fprintf(stderr,"\tCreating table %s\n","AnomalyCurve");
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_AnomalyCurve);
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

  {"help", no_argument, NULL, 'h'},
  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("NCD Snapshot V.001 - Sets Anomaly Curve.\n");
    printf("Usage: ./anomaly [OPTIONS] filename \n");
    printf("\t-Q, --query-num\t\t Query number to use.\n");
	printf("\t-q, --query\t	 Use the last query created.\n");
    printf("\t-H, --host=name\t\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-S, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
    printf("\t-h, --help\t\t This help page.\n");
  
}
#define PASS_SIZE 100
int main(int argc, char *argv[] )
{
	char values[BUFFER];  
    char *host_name = NULL;
    char *user_name = NULL;
    char *password = NULL;
    char tempstring[20];
    char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
    unsigned int port_num = 0;
    char *socket_name = NULL;
    int input,option_index,key;
    char scandir[PATH_MAX];
    MYSQL *conn = NULL;
    char query[BIGBUFFER];
	FILE *csvfile;
	char readbuffer[BUFFER];
    char INfile[PATH_MAX];
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
			  
	    }//switch      
     
    }//while
    
    //Move to end of Argument List

	argc -= optind; 
    argv += optind;
	if(argc == 0)
	{
		fprintf(stderr,"Need a CSV File\n"); exit(1);
	}
	else if(argc == 1)
	{
		strncpy(INfile,argv[0],PATH_MAX);
		if(checkfile(INfile) != 1) { fprintf(stderr,"Invalid File\n"); exit(1); }

	}
	else { fprintf(stderr,"Need a CSV File \n"); exit(1); }
	
	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
    //Setup SQL  
    conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	csvfile = fopen(INfile, "r");
	if (csvfile == NULL) { fprintf(stderr,"Error opening File\n"); exit(1); }

	initTables(conn);	
	printf("Adding values from %s to Anomaly Curve Table.\n",INfile);
	
	while (!feof(csvfile))
	{
		if(fgets(readbuffer, BUFFER, csvfile) != NULL)
		{
			getValues(readbuffer, values, 2);		
			snprintf(query, BUFFER, "INSERT INTO %s %s VALUES %s","AnomalyCurve", "(number, anomaly)", values);

			if (mysql_query(conn,query) != 0)
				mysql_print_error(conn);
		}
	}
	
	if (mysql_query(conn,"COMMIT;") != 0)
		mysql_print_error(conn);     
	
	printf("Completed\n");
     //Sql End
      mysql_close(conn);
    
    return(0);
}


