#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <mysql/mysql.h>
#include "mysql.h"
#define BIGBUFFER 10000

#define db_name "SNAPSHOT"
#define DRIVE_TABLENAME "image_snapshot_table"
#define IMAGE_LIST_TABLENAME "image_list_table"




void printlisttemp(struct dirDFS *current)
{
  printf("List of items.\n");
  	while(current != NULL)
	{
	    printf("%s\n",current->dirname);
	    current = current->next;	  
	}
}

static const char *groups[] = {"client", 0};

struct option long_options[] =
{
  {"host", required_argument, NULL, 'H'},
  {"user", required_argument, NULL, 'u'},
  {"password", required_argument, NULL, 'p'},
  {"port", required_argument, NULL, 'P'},
  {"socket", required_argument, NULL, 's'},
  {"file", required_argument, NULL, 'F'},
  {"screen", no_argument, NULL, 'S'},
  {"help", no_argument, NULL, 'h'},
  { 0, 0, 0, 0 }
}; //long options

void printhelp()
{
    printf("Snapshot V.001 - Takes snapshot of Files and places them into SQL.\n");
    printf("Defaults may be stored in your home directory in ./.snapshot.cnf\n");
    printf("Usage: ./snapshot [OPTIONS] DIRECTORY_PATH \n");

    printf("\t-H, --host=name\t Connect to MySQL Host Name\n");
    printf("\t-u, --user=name\t Connect to MySQL using User Name\n");
    printf("\t-p, --password=password\t Connect to MYSQL using password\n");
    printf("\t-P, --port=[#]\t Connect to MySQL using port number # (other then default)\n");
    printf("\t-s, --socket=file\t Connect to MYSQL using a UNIX Socket\n");
	printf("\t-I, --intialize\t Intialize new tables (Warning: wipes/drops old).\n");
    printf("\t-f, --file=name\t Insted of MySQL - Dumps Insert statements into file: name\n");
    printf("\t-S, --screen\t Dumps SQL INSERT commands to STDOUT\n");


    printf("\t-h, --help\t This help page.\n");
  
}
#define PASS_SIZE 100
int main(int argc, char *argv[] )
{
  
    char *host_name = NULL;
    char *user_name = NULL;
    char *password = NULL;
    char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
    unsigned int port_num = 0;
    char *socket_name = NULL;
    int input,option_index,key;
    FILE *fileoutput = NULL;
    char OUTPUT_PATH[PATH_MAX];
    MYSQL *conn = NULL;
    
    //Setup for MySQL Init File
    my_init();
    
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "H:u:p:P:s:SF:hnIi:c:m:N", long_options, &option_index)) != EOF )
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
	case 'i' :
	  investigator = optarg;
	  break;
	case 'c' :
	  case_name = optarg;
	  break;
	case 'm' :
	  image_name = optarg;
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
	case 's' :
	  socket_name = optarg;  
	  break;
	case 'S' :
	  //GLOB_PRINT_FLAG = 1;
	  //GLOB_SQL = 0;
	  fprintf(stderr,"Sorry, this program needs access to the database. This option is not valid.\n");
	  exit(1);
	  break;
	case 'F' :
	  //GLOB_FILE_FLAG = 1;
	  //GLOB_SQL = 0;
	  //snprintf(OUTPUT_PATH,PATH_MAX,"%s\0",optarg);
	  fprintf(stderr,"Sorry, this program needs access to the database. This option is not valid.\n");
	  exit(1);
	  break;
		  
      }//switch      
     
    }//while
    
    //Move to end of Argument List
    argc -= optind; 
    argv += optind;
    
    //Used CWD if nothing else specified
    if(argc == 0)
    {
       //print help
    }
    else
    {
     //strncpy(scandir,argv[0],PATH_MAX); 
    }
    
    //Setup Output
    if(GLOB_SQL == 1)
    {
     //Setup SQL  
     conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
    }

	//Output files into SQL or Screen or File
	//doquery(directory, fileoutput, conn, key);
    
    //Close Output
    if(GLOB_FILE_FLAG == 1)
    {
      //Close Output File
       fclose(fileoutput);
    }
    if(GLOB_SQL == 1)
    {
     //Sql End
     if (mysql_query(conn,"COMMIT;") != 0)
			mysql_print_error(conn);
      mysql_close(conn);
    }
    
    return(0);
}


