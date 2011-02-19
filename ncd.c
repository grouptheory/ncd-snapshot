#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
//#include <mysql/my_sys.h>
#include <getopt.h>
#include <zlib.h>
#include <math.h>
#include "mysql.h"
#define BIGBUFFER 10000

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
void ncdsnapshot(MYSQL *, MYSQL *, int, int, ssize_t);
int compressSize(char *, ssize_t *, ssize_t *, ssize_t *, int , int, ssize_t );
int combineSize(char *, char *, ssize_t *, ssize_t *, ssize_t *, int , int, ssize_t );
void NCDtwofiles(char *, char *, int , int, ssize_t, float *, float * );
void NCDtwofilesRand(char *, char *, int, int, int, float *, float *);
int checkfile(const char *);
void printhelp();
ssize_t getFileSize(char *);
void showTableQuery(MYSQL *, int, int);
int pickSerial(MYSQL *);

//Flags
int GLOB_INIT_FLAG = 0; //Initialize Tables - Default NO
int GLOBAL_VERBOSE = DVERBOSE;
int GLOBAL_SHOWMATCHING = 0;
int GLOBAL_SECONDSTAGE = 0;
int GLOBAL_DOUBLE = 0;
int GLOBAL_CVSOUT = 0;
float GLOBAL_BASE = 0;
int GLOBAL_RANDOM = 0;
int GLOBAL_RANDOMK = 0;
int GLOBAL_SHOWOUT = 0;
int GLOBAL_COMPARE = 0;
int GLOBAL_NEWQUERY = 0;
int TINY_CHUNK = DEFAULT_TINYCHUNK;

char cursor[4]={'/','-','\\','|'}; //For Spinning Cursor



int compressSize(char *file, ssize_t *osize, ssize_t *csize, ssize_t *dsize, int level, int CHUNK, ssize_t offset)
{
	int ret,bound;
	unsigned long size;
	Bytef input[CHUNK];
	Bytef *output;
	FILE *source;
	
	if( (source = fopen(file, "rb")) == NULL) return Z_ERRNO;
	
	if(offset > 0) if( (fseek(source, offset, SEEK_SET)) != 0) {fprintf(stderr,"Error seeking file %s.\n",file); exit(1);}
	
	*osize = fread(input, 1, CHUNK, source);
	if (ferror(source)) 
	{
	    return Z_ERRNO;
	}

	
	size = compressBound(*osize);
	output = malloc(sizeof(Bytef)*size);
	ret = compress2(output, &size, input, *osize, level);
	*csize =size;

	//Optional Stage Two
	if(GLOBAL_DOUBLE == 1)
	{
	  Bytef *secondoutput;
	  size = compressBound(*csize);
	  if(TINY_CHUNK <  *csize) *csize = TINY_CHUNK;
	  secondoutput = malloc(sizeof(Bytef)*size);
	  ret = compress2(secondoutput, &size, output, *csize, level);
	  *dsize = size;
	  free(secondoutput);
	}
	
	free(output);
	fclose(source);
    return Z_OK;
}//compressSize

int combineSize(char *file, char *filetwo, ssize_t *osize, ssize_t *csize, ssize_t *dsize, int level, int CHUNK, ssize_t offset)
{
	int ret,bound;
	unsigned long size;
	Bytef input[CHUNK*2];

	Bytef *output;
	FILE *source;
	FILE *sourcetwo;
	ssize_t firstsize;
	
	if( (source = fopen(file, "rb")) == NULL) return Z_ERRNO;
	if(offset > 0) if( (fseek(source, offset, SEEK_SET)) != 0) {fprintf(stderr,"Error seeking file %s.\n",file); exit(1);}
	firstsize = fread(input, 1, CHUNK, source);
	if (ferror(source)) {
	    return Z_ERRNO;
	}
	fclose(source);
	
	if( (sourcetwo = fopen(filetwo, "rb")) == NULL) return Z_ERRNO;
	if(offset > 0) if( (fseek(sourcetwo, offset, SEEK_SET)) != 0) {fprintf(stderr,"Error seeking file %s.\n",file); exit(1);}
	*osize = fread( &input[firstsize], 1, CHUNK, source);
	if (ferror(source)) {
	    return Z_ERRNO;
	}
	*osize = *osize + firstsize;
	fclose(sourcetwo);
	
	size = compressBound(*osize);
	output = malloc(sizeof(Bytef)*size);
	ret = compress2(output, &size, input, *osize, level);
	*csize =size;
	
	//Optional Stage Two
	if(GLOBAL_DOUBLE == 1)
	{
	  Bytef *secondoutput;
	  size = compressBound(*csize);
	  if(TINY_CHUNK <  *csize) *csize = TINY_CHUNK;
	  secondoutput = malloc(sizeof(Bytef)*size);
	  ret = compress2(secondoutput, &size, output, *csize, level);
	  *dsize = size;
	  free(secondoutput);
	}

    free(output);
    return Z_OK;
}//compressSize

/*
NCD Formula
NCD(x,y)  =  C(xy) - min{C(x), C(y)} / max{C(x), C(y)},  
*/
void NCDtwofiles(char *file1, char *file2, int type, int CHUNK, ssize_t offset, float *ncd, float *dncd)
{
	ssize_t compressfile1, compressfile2, combined;
	ssize_t dcompressfile1, dcompressfile2, dcombined;
	ssize_t original;
	ssize_t *min;
	ssize_t *max;
	compressSize(file1, &original, &compressfile1, &dcompressfile1, type, CHUNK, offset);
	compressSize(file2, &original, &compressfile2, &dcompressfile2, type, CHUNK, offset);
	combineSize(file1, file2, &original, &combined, &dcombined, type, CHUNK, offset);
	
	min = compressfile1 <= compressfile2 ? &compressfile1 : &compressfile2;
	max = compressfile1 >= compressfile2 ? &compressfile1 : &compressfile2;	
	*ncd = ( (float)(combined - *min) / (float)*max);
	
	if(GLOBAL_DOUBLE == 1)
	{
		min = dcompressfile1 <= dcompressfile2 ? &dcompressfile1 : &dcompressfile2;
		max = dcompressfile1 >= dcompressfile2 ? &dcompressfile1 : &dcompressfile2;	
		*dncd = ( (float)(dcombined - *min) / (float)*max);
	}
	else *dncd = 0;

}

/*
Open File Seek to End and Return location

*/
ssize_t getFileSize(char *filename)
{
   int fd;
   ssize_t offset;
   if( (fd = open(filename, O_RDONLY)) == -1 ) {fprintf(stderr,"Error opening original file %s (Not Fatal - Will try to skip it)\n",filename); return 0;}
   offset = lseek(fd, 0, SEEK_END);
   if(offset == -1) {fprintf(stderr,"Error seeking file %s.\n",filename); exit(1);}
   close(fd);
   return offset;
}


/*
NCD Formula
NCD(x,y)  =  C(xy) - min{C(x), C(y)} / max{C(x), C(y)},  

This function does not look at the first byes of a file but instead randomly checks a different position in a file.
This can also be looped and the results added together.

*/
void NCDtwofilesRand(char *file1, char *file2, int type, int CHUNK, int rotation, float *ncd, float *dncd)
{
	ssize_t compressfile1, compressfile2, combined;
	ssize_t dcompressfile1, dcompressfile2, dcombined;
	ssize_t original;
	ssize_t *min;
	ssize_t *max;
	ssize_t maxfile,maxfilesize,offset;
	int count = 0;
	
	maxfile = getFileSize(file1);
	maxfilesize = getFileSize(file2);
	maxfilesize = maxfile < maxfilesize ? maxfile : maxfilesize;
	//This check and sentinel value should be okay since we shouldn't have a negative NCD value. Code below will ignore this value.
	if(maxfilesize == 0) *ncd = -999;
	else
	{
		maxfilesize -= CHUNK;
		
		//will run once if rotation is 0 or 1
		do
		{
			offset = ((((double)rand()) / ((double)(RAND_MAX) + (double)1))*maxfilesize);
		  
			compressSize(file1, &original, &compressfile1, &dcompressfile1, type, CHUNK, offset);
			compressSize(file2, &original, &compressfile2, &dcompressfile2, type, CHUNK, offset);
			combineSize(file1, file2, &original, &combined, &dcombined, type, CHUNK, offset);

			min = compressfile1 <= compressfile2 ? &compressfile1 : &compressfile2;
			max = compressfile1 >= compressfile2 ? &compressfile1 : &compressfile2;	 
			*ncd = *ncd + ( (float)(combined - *min) / (float)*max);
			if(GLOBAL_DOUBLE == 1)
			{
				min = dcompressfile1 <= dcompressfile2 ? &dcompressfile1 : &dcompressfile2;
				max = dcompressfile1 >= dcompressfile2 ? &dcompressfile1 : &dcompressfile2;	
				*dncd = *dncd + ( (float)(dcombined - *min) / (float)*max);
			}
			else *dncd = 0;
			count++;
		}
		while(count < rotation);
	}
}//Two File NCD w/ random location

void ncdsnapshot(MYSQL *connread, MYSQL *connwrite, int query_num, int chunk, ssize_t offset)
{

	char sqlbuffer[BIGBUFFER+2];
	char second_sqlbuffer[BIGBUFFER+2];
	char file_one[PATH_MAX];
	char file_two[PATH_MAX];
	MYSQL_RES *res;
	MYSQL_ROW row;
	float ncd = 0;
	float dncd = 0;
	short int spot;	
	int commitcount = 0;
	snprintf(sqlbuffer, BIGBUFFER,"DELETE QUICK IGNORE FROM NCD_result USING NCD_result, NCD_table WHERE (NCD_result.ncd_key = NCD_table.ncd_key) AND (NCD_table.querynumber = %d);",query_num);

	fprintf(stderr,"Removing any previous NCD values related to query [%d].\n", query_num);
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);
	while( (row = mysql_fetch_row(res)) != NULL);
	printf("Running a COMMIT\n");
	if (mysql_query(connwrite,"COMMIT;") != 0)
		mysql_print_error(connwrite);
	
	snprintf(sqlbuffer, BIGBUFFER,"select n.ncd_key, CONCAT(a.directory,'/', a.filename), CONCAT(b.directory, '/', b.filename) FROM image_snapshot_table AS a JOIN image_snapshot_table AS b JOIN NCD_table AS n ON n.file_one = a.item AND n.file_two = b.item AND n.querynumber = %d",query_num);
	
	
	fprintf(stderr,"Inserting NCD Values into NCD result table.\n");
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);
	while( (row = mysql_fetch_row(res)) != NULL)
	{
		//strncpy(file_one, row[1], PATH_MAX);
		//strncpy(file_two, row[2], PATH_MAX);
		ncd = 0; dncd =0;
		if(GLOBAL_RANDOM == 0)
			NCDtwofiles(row[1], row[2], Z_DEFAULT_COMPRESSION, chunk, offset, &ncd, &dncd);
		else 
			NCDtwofilesRand(file_one, file_two, Z_DEFAULT_COMPRESSION, chunk, GLOBAL_RANDOMK, &ncd, &dncd);
	    if (ncd == -999) continue; //skip if we had an error
	    snprintf(second_sqlbuffer, BIGBUFFER, "INSERT INTO %s VALUES (\"%s\", \"%f\", \"%f\");", NCD_RESULT_TABLENAME, row[0], ncd, dncd);
		//printf("Test: 1:%s 2:%s strlen %d Full %s\n", row[1], row[2], strlen(row[2]), second_sqlbuffer);
    if (mysql_query(connwrite,second_sqlbuffer) != 0)
			mysql_print_error(connwrite);		
				commitcount++;
		if (commitcount >= 10000) 
		{
			if (mysql_query(connwrite,"COMMIT;") != 0)
			mysql_print_error(connwrite);
			commitcount = 0;
			sync();
		}
	    if(!GLOBAL_VERBOSE) 
	    {
	      printf("%c\b", cursor[spot]); 
	      fflush(stdout);
	      spot = (spot+1) % 4;
	    }
		//else printf("%s\n",second_sqlbuffer);
	    
	 }
	printf("Running a COMMIT\n");
	if (mysql_query(connwrite,"COMMIT;") != 0)
		mysql_print_error(connwrite);

	 fprintf(stderr,"NCD Values have been added to the NCD Result Table.\n");
	 
}//ncdsnapshot



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
    printf("\t-w, --show\t Show NCD Table when completed\n");
    printf("\t-l, --limit\t Limit the output of the show command.\n");
    printf("\t\t\t NCD Options\n");
    printf("\t-c, --chunk\t The value of bytes that we check,\n\t\t\t Default is currently set to: %d\n",DEFAULT_CHUNK);
    printf("\t-o, --random\t Use a random offset to compare.\n");
    printf("\t-k, --repeat\t Optional argument to be used with --random option.\n\t\t\t Repeats operation K times. EX: -k 2.\n");
    printf("\t-O, --offset\t Use a specifc offset in file (Bytes). EX: -O 256\n");
	printf("\t-D, --double\t Complete a double NCD of each selection\n");
	printf("\t-T, --tinychunk\t Change the default size of the small chunk size.\n\t\t\t Default is currently set to: %d\n", DEFAULT_TINYCHUNK);
	
    
    printf("\t-h, --help\t This help page.\n");
  
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
	char *query_number = NULL;
	int query_num = 0;
    int limit_value = 0;
    int krepeat = 0;
	int item_num;
    int chunk = DEFAULT_CHUNK;
    ssize_t offset = 0;
 
    
    
    //Setup For OpenSSL Hash
    OpenSSL_add_all_digests();
    
    //Setup for MySQL Init File
    my_init();
    
    //Load Defaults -- adds file contents to arugment list - Thanks MySql
    load_defaults("snapshot", groups, &argc, &argv);
    
    while((input = getopt_long(argc, argv, "hH:u:Q:qp:P:S:Dc:O:owl:T:k:", long_options, &option_index)) != EOF )
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
	case 'D' :
	  GLOBAL_DOUBLE = 1;
	  break;	  
	case 'c' :
	  strncpy(tempstring,optarg,19);
	  chunk = atoi(tempstring);
	  if(chunk < 8000) { fprintf(stdout,"Bad value entered.\n"); exit(1); }
	break;
	case 'O' :
	  strncpy(tempstring,optarg,19);
	  offset = atoi(tempstring);
	  if(offset < 0) { fprintf(stdout,"Bad offset value entered.\n"); exit(1); }
	break;
	case 'o' :
	  GLOBAL_RANDOM = 1;
	break;
	case 'w' :
	  GLOBAL_SHOWOUT = 1;
	break;
	case 'l' :
	  strncpy(tempstring,optarg,19);
	 limit_value = atoi(tempstring);
	  if(limit_value < 0) { fprintf(stdout,"Bad limit value entered.\n"); exit(1); }
	break;
	case 'T' :
		strncpy(tempstring,optarg,19);
		TINY_CHUNK = atoi(tempstring);
		if(TINY_CHUNK < 1000) { fprintf(stdout,"Bad tiny chunk value entered.\n"); exit(1); }
	break;
	case 'k' :
	  strncpy(tempstring,optarg,19);
	  GLOBAL_RANDOMK = atoi(tempstring);
	  if(krepeat < 0) { fprintf(stdout,"Bad value entered.\n"); exit(1); }
	  GLOBAL_RANDOMK = krepeat;
	break;
		  
      }//switch      
     
    }//while
    
    //Move to end of Argument List
    argc -= optind; 
    argv += optind;
	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
    
	connread = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	connwrite = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	query_num = pickQuery(connread, query_number);
	if(query_num > 0)
	{
		ncdsnapshot(connread, connwrite, query_num, chunk, offset);
		if(GLOBAL_SHOWOUT == 1)
		{
			showTableQuery(connread, query_num, limit_value);
		}
	}

	//Sql End
	if (mysql_query(connwrite,"COMMIT;") != 0)
		mysql_print_error(connwrite);
	mysql_close(connread);
	mysql_close(connwrite);

    
    return(0);
}


