#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
#include <getopt.h>
#include "mysql.h"
#include<setjmp.h>
#include <signal.h>
#include <time.h>
#include <pthread.h>

#define ARRAYSIZE_TIME 10
#define ARRAYSIZE_IMAGES 20
#define BIGBUFFER 10000
#define sqlbufsize 255
#define db_name "SNAPSHOT"
#define DRIVE_TABLENAME "image_snapshot_table"
#define IMAGE_LIST_TABLENAME "image_list_table"
#define NCD_TABLENAME "NCD_table"
#define QUERY_TABLENAME "query_table"
#define NCD_RESULT_TABLENAME "NCD_result"
#define PASS_SIZE 100

struct storage_struct {
	long int min;
	long int max;
	pthread_t TID; //thread ID
};

void printhelp();
static void sig_catch(int);
static sigjmp_buf jmpbuffer;

//MySQL Login Information
char *host_name = NULL;
char *user_name = NULL;
char *password = NULL;
unsigned int port_num = 0;
char *socket_name = NULL;

static void sig_catcher(int sig_no)
{
	fprintf(stderr,"OUCH: Signal %d Received\n", sig_no);
	siglongjmp(jmpbuffer, 1);	
	
}//signal Catch

//Flags
int GLOB_INIT_FLAG = 0; //Initialize Tables - Default NO
int GLOBAL_THREADS = 4; //
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
float array[ARRAYSIZE_IMAGES+1][ARRAYSIZE_IMAGES+1][ARRAYSIZE_TIME+1];

char *stimeStamp(char *timeS)
{	
	time_t epoch;
	time_t *epoch_ptr;
	char timebuf[100];
	char *timebuf_ptr = timebuf;
	epoch_ptr = &epoch;
	if (time(epoch_ptr) == -1) {fprintf(stderr,"Error getting TOD.\n"); exit(2); }
	strftime(timebuf, 100, "%b %d %T",localtime(epoch_ptr));
	strftime(timeS, 100, "%b %d %T",localtime(epoch_ptr));
	//return strtok(asctime(localtime(epoch_ptr)), "\n");
	return timebuf_ptr;
}


void initTables(MYSQL *conn, int time)
{
	if(conn == NULL) exit(1);
	MYSQL_RES *res;
	MYSQL_ROW row;
	char sqlbuffer[BIGBUFFER];
	
	/*snprintf(sqlbuffer,BIGBUFFER,"UPDATE AnomalyConfig SET query = %d;",time);
		if (mysql_query(conn,sqlbuffer) != 0)
			mysql_print_error(conn);
	*/
	// Anomaly Temps
	if(GLOB_INIT_FLAG) fprintf(stderr,"\tCreating temporary table %s\n","AnomalyQueryStart_Temp;");
	snprintf(sqlbuffer,BIGBUFFER,"DROP TABLE IF EXISTS AnomalyQueryStart_Temp;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE AnomalyQueryStart_Temp AS select a.image AS IMG1, n.file_one AS F1, b.image AS IMG2, n.file_two AS F2, ncd_normal FROM NCD_table AS n JOIN NCD_result AS r JOIN image_snapshot_table AS a JOIN image_snapshot_table AS b ON n.ncd_key = r.ncd_key AND n.file_one = a.item AND n.file_two = b.item AND n.querynumber = %d AND ncd_normal <= %f ORDER BY IMG1, F1;", time, CUTOFF);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	res = mysql_use_result(conn);
	
		if(GLOB_INIT_FLAG) fprintf(stderr,"\tCreating temporary table %s\n","Anomaly_Number_Collabrators_Temp");
	snprintf(sqlbuffer,BIGBUFFER,"DROP TABLE IF EXISTS Anomaly_Number_Collabrators_Temp;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE Anomaly_Number_Collabrators_Temp AS select DISTINCT IMG1, F1, COUNT(IMG2) As NUM FROM AnomalyQueryStart_Temp GROUP BY F1;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	res = mysql_use_result(conn);
	
		if(GLOB_INIT_FLAG) fprintf(stderr,"\tCreating temporary table %s\n","AnomalyJoin_Temp");
	snprintf(sqlbuffer,BIGBUFFER,"DROP TABLE IF EXISTS AnomalyJoin_Temp;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE AnomalyJoin_Temp AS SELECT A.IMG1, A.F1, C.anomaly FROM Anomaly_Number_Collabrators_Temp AS A JOIN AnomalyCurve AS C ON A.NUM = C.number;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	res = mysql_use_result(conn);
	if(GLOB_INIT_FLAG) fprintf(stderr,"\tCreating temporary table %s\n","AnomalyJoin_Temp2");
	snprintf(sqlbuffer,BIGBUFFER,"DROP TABLE IF EXISTS AnomalyJoin_Temp2;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE AnomalyJoin_Temp2 AS SELECT A.IMG1, A.F1, C.anomaly FROM Anomaly_Number_Collabrators_Temp AS A JOIN AnomalyCurve AS C ON A.NUM = C.number;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	res = mysql_use_result(conn);
	
	//Anomaly Temps
	
	
	if(GLOB_INIT_FLAG) fprintf(stderr,"\tCreating temporary table %s\n","Collaborative_Start_Temp");
	snprintf(sqlbuffer,BIGBUFFER,"DROP TABLE IF EXISTS Collaborative_Start_Temp;");
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE Collaborative_Start_Temp AS SELECT DISTINCT S.IMG1 AS IMG1, S.F1 AS F1, J.anomaly AS A1, S.IMG2 AS IMG2, S.F2 AS F2, J2.anomaly AS A2, S.ncd_normal AS NCD, ROUND((J.anomaly + J2.anomaly),3) AS AnomalySUM FROM AnomalyQueryStart_Temp AS S LEFT JOIN AnomalyJoin_Temp AS J USING(IMG1, F1) LEFT JOIN AnomalyJoin_Temp2 AS J2 ON S.IMG2 = J2.IMG1 AND S.F2 = J2.F1;");
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
	if(time == 0) fprintf(stderr,"WTF with Time!\n");
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);
	
	while( (row = mysql_fetch_row(res)) != NULL)
	{
			image_one = atoi(row[0]);
			image_two = atoi(row[1]);
			collaboration_num = atof(row[2]);
			array[image_one][image_two][time] = collaboration_num;
			//fprintf(stderr,"T%d: %d,%d,%f\n",time,image_one,image_two,collaboration_num);
	}
	
}

void * getThread(void *parm)
{
	//creat data structure
	struct storage_struct *data;
	//Parm is a void of any structure, form parm into actual data structure 
	data = (struct storage_struct *)parm;
	MYSQL *conn = NULL;
	int z;
  	conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	if(conn == NULL) { fprintf(stderr,"[%lu] Error opening MySQL Connection.\n",data->TID); exit(1); }
	
	for(z = data->min; z <= data->max; z++)
	{
		printf("%s : Getting data for iteration %d of %d.\n",stimeStamp(NULL), z,ARRAYSIZE_TIME);
		fflush(stdout);
		initTables(conn, z);
		getTabledata(conn, "Collaborative_Result_Temp", 0, z);
	}
	mysql_close(conn);
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

int main(int argc, char *argv[] )
{
  
    char tempstring[20];
    int input,option_index,key;
    FILE *fileoutput = NULL;
    int limit_value = 0;
	int x,y,z,q, check,first;
    char sqlbuffer[BIGBUFFER];
	char tempbuffer[BIGBUFFER];
	char plotYbuffer[BIGBUFFER];
	char plotNamebuffer[BIGBUFFER];
	char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
  	int min, max, total, start, end, modulus;
	int thread_count = GLOBAL_THREADS;
	int c;
	MYSQL *conn = NULL;
	struct storage_struct thread_storage[GLOBAL_THREADS]; //create global container
	pthread_attr_t attr;
	//get default attributes 
	pthread_attr_init(&attr);
	//Must be set to wait for Threads to finish
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
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
	
	FILE *outhcsv;
	if( (outhcsv= fopen("output-h.csv", "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n","output-h.csv"); }
	FILE *outvcsv;
	if( (outvcsv= fopen("output-v.csv", "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n","output-v.csv"); }
	FILE *plotdata;
	if( (plotdata= fopen("ploticus-options", "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n","ploticus-options"); }
	
	FILE *gnuplotdata;
	if( (gnuplotdata= fopen("plot.dat", "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n","plot.dat"); }
	FILE *gnuplot;
	if( (gnuplot= fopen("plot.cmd", "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n","plot.cmd"); }
	
	sigset_t myset;
	sigfillset(&myset);
	sigdelset(&myset, SIGTERM);
	sigprocmask(SIG_BLOCK, &myset,NULL);
	
	if(signal(SIGTERM,sig_catcher) == SIG_ERR) { fprintf(stderr,"Error Setting up Signal Catcher!\n"); exit(2);}
	conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	if(conn == NULL) { fprintf(stderr,"[Main] Error opening MySQL Connection.\n"); exit(1); }
   
	//Setting up main array
	printf("Setting up Array for %d Images and %d iterations.\n", ARRAYSIZE_IMAGES, ARRAYSIZE_TIME);
	for(x =0; x <= ARRAYSIZE_IMAGES; x++)
		for(y=0; y <= ARRAYSIZE_IMAGES; y++)
			for(z=0; z <= ARRAYSIZE_TIME; z++)
				array[x][y][z] = 0;
	
	//Get Data
	printf("Starting to get data.\n");
	min = 1;
	max = ARRAYSIZE_TIME;
	
	if( (sigsetjmp(jmpbuffer,1) == 0))
	{	
		//Fill in Min/Max
		for(c = 0; c < GLOBAL_THREADS; c++)
		{
			if(c == 0) 
			{ 
				modulus = ARRAYSIZE_TIME / GLOBAL_THREADS; 
				if(modulus == 0)
				{
					//More threads then total number? Ouch - only open one thread
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
			pthread_create(&thread_storage[c].TID, &attr, getThread, (void *) &thread_storage[c]);
			printf("Starting thread[%lu] %ld of %ld to gather data (%d - %d)\n", thread_storage[c].TID, c+1, thread_count, thread_storage[c].min, thread_storage[c].max);
		}	
		
		//Wait for Threads to finish
		for (c=0; c < thread_count; c++)
		{
			pthread_join(thread_storage[c].TID, NULL);
		}
	
	
	}
	
	//Output to file
	fprintf(stderr,"Writting file.\n");
	//Horizontal output
	for(x=0; x <= ARRAYSIZE_IMAGES; x++)
		for(y=0; y <= ARRAYSIZE_IMAGES; y++)
		{
			//Print Pair
			check = 0;
			for(z=1; z <= ARRAYSIZE_TIME; z++) 
			{
				//Check for anything
				if(array[x][y][z] != 0) check = 1;			
			}
			if(check == 1) 
			{
				fprintf(outhcsv,"%d, %d",x,y);
				for(z=1; z <= ARRAYSIZE_TIME; z++)
				{
					//print values
					fprintf(outhcsv,", %f",array[x][y][z]);
				}
				fprintf(outhcsv,"\n");
			}
		}
	//Vertical Output
	//Header
	fprintf(outvcsv,"#");
	for(x=0; x <= ARRAYSIZE_IMAGES; x++)
		for(y=0; y <= ARRAYSIZE_IMAGES; y++)
		{
			//Print Pair
			check = 0;
			for(z=1; z <= ARRAYSIZE_TIME; z++) 
			{
				//Check for anything
				if(array[x][y][z] != 0) check = 1;			
			}
			if(check == 1) 
			{
				fprintf(outvcsv,", %d-%d",x,y); 
			}
		}
	fprintf(outvcsv,"\n");
	
	//Data
	for(z=1; z < ARRAYSIZE_TIME; z++) 
	{
		fprintf(outvcsv,"%d", (z+1));
		for(x=0; x <= ARRAYSIZE_IMAGES; x++)
		for(y=0; y <= ARRAYSIZE_IMAGES; y++)
		{
			//Print Pair
			check = 0;
			for(q=1; q <= ARRAYSIZE_TIME; q++) 
			{
				//Check for anything
				if(array[x][y][q] != 0) check = 1;			
			}
			if(check == 1) 
			{
				//print values
				fprintf(outvcsv,", %f",array[x][y][z]);
			}
		}
		fprintf(outvcsv,"\n");
	}
	
	//Ploticus parameters
	q=1;
	sprintf(plotYbuffer,"x=1 ");
	sprintf(plotNamebuffer,"");
	for(x=0; x <= ARRAYSIZE_IMAGES; x++)
		for(y=0; y <= ARRAYSIZE_IMAGES; y++)
		{
			//Print Pair
			check = 0;
			for(z=1; z <= ARRAYSIZE_TIME; z++) 
			{
				//Check for anything
				if(array[x][y][z] != 0) check = 1;			
			}
			if(check == 1) 
			{
				if(q==1) snprintf(tempbuffer,BIGBUFFER,"name=%d-%d ",x,y); 
				else snprintf(tempbuffer,BIGBUFFER,"name%d=%d-%d ",q,x,y); 
				strncat(plotNamebuffer, tempbuffer, BIGBUFFER);
				if(q==1) snprintf(tempbuffer,BIGBUFFER,"y=%d ",q+1); 
				else snprintf(tempbuffer,BIGBUFFER,"y%d=%d ",q,q+1); 
				strncat(plotYbuffer, tempbuffer, BIGBUFFER);
				q++;
			}
		}
	fprintf(plotdata,"%s %s", plotYbuffer, plotNamebuffer);
	
	//GnuPLOT  Commands
	q=2;
	fprintf(gnuplot,"set term png\n");
	fprintf(gnuplot,"set output \"graphic.png\"\n");
	fprintf(gnuplot,"set xlabel 'Iterations'\n");
	fprintf(gnuplot,"set ylabel 'NCD'\n");
	fprintf(gnuplot,"set title 'Simulation'\n");
	for(x=0; x <= ARRAYSIZE_IMAGES; x++)
		for(y=0; y <= ARRAYSIZE_IMAGES; y++)
		{
			//Print Pair
			check = 0;
			for(z=1; z <= ARRAYSIZE_TIME; z++) 
			{
				//Check for anything
				if(array[x][y][z] != 0) check = 1;			
			}
			if(check == 1) 
			{
				if(q==2) fprintf(gnuplot,"plot \"plot.dat\" using 1:%d title \"%d-%d\" with linespoints",q,x,y);
				else fprintf(gnuplot,", \"plot.dat\" using 1:%d title \"%d-%d\" with linespoints",q,x,y); 
				q++;
			}
		}
	fprintf(gnuplot,"\n");
	fprintf(gnuplot,"quit\n");
	
	//GNU Data
	for(z=1; z <= ARRAYSIZE_TIME; z++) 
	{
		fprintf(gnuplotdata,"%d\t", z);
		for(x=0; x <= ARRAYSIZE_IMAGES; x++)
		for(y=0; y <= ARRAYSIZE_IMAGES; y++)
		{
			//Print Pair
			check = 0;
			for(q=1; q <= ARRAYSIZE_TIME; q++) 
			{
				//Check for anything
				if(array[x][y][q] != 0) check = 1;			
			}
			if(check == 1) 
			{
				//print values
				fprintf(gnuplotdata,"%f\t",array[x][y][z]);
			}
		}
		fprintf(gnuplotdata,"\n");
	}
	
    //Sql End
	fclose(outvcsv);
	fclose(outhcsv);
	fclose(plotdata);
	fclose(gnuplotdata);
	fclose(gnuplot);
   mysql_close(conn);
    return(0);
}


