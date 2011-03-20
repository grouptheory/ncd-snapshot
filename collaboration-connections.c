#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <mysql/mysql.h>
#include <getopt.h>
#include "mysql.h"
#include <time.h>


#define ARRAYSIZE_TIME 100
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

void printhelp();
void BFS(int **, int);

//MySQL Login Information
char *host_name = NULL;
char *user_name = NULL;
char *password = NULL;
unsigned int port_num = 0;
char *socket_name = NULL;

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

//Simple Queue

struct QueueNode
{
	int QueueItem;
	struct QueueNode *next;
};
typedef struct QueueNode QueueNode;

QueueNode *Qhead;
QueueNode *Qtail;
		
int QisEmpty() { return (Qtail == NULL);}

void enqueue(const int item)
{
	QueueNode *newNode = malloc(sizeof(QueueNode)); //allocate node and memory
	
	newNode->QueueItem = item; //copy item into newNode data space
	newNode->next = NULL; //assign Null pointer to newNode
	
	if(QisEmpty()) Qhead = newNode; 
	else Qtail->next = newNode; 
	
	//have new node become tail
	Qtail = newNode;
	/**/
}//end enqueue

int dequeue(int *item)
{
	if(!QisEmpty())
	{
		*item = Qhead->QueueItem;
		if(Qhead == Qtail) { Qhead = Qtail = NULL;} //special case to empty out queue
		else
		{
			QueueNode *zombie;
			zombie = Qhead;
			Qhead = Qhead->next;
			free(zombie);
		}
		return 0;
	}
	return -1;
}//end dequeue

cleanQueue()
{
	int temp;
	while(!QisEmpty())
		dequeue(&temp);
}//end destructor


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
	//snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE Collaborative_Result_Temp AS select IF(MOD(IMG1,%d) = 0, %d, MOD(IMG1,%d)) AS IMG1, IF(MOD(IMG2,%d) = 0, %d, MOD(IMG2,%d)) AS IMG2, SUM(AnomalySUM) As SUM FROM Collaborative_Start_Temp WHERE AnomalySUM > 0 AND NCD < %f GROUP BY IMG1,IMG2;",MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE,CUTOFF);
	//snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE Collaborative_Result_Temp AS select IF(MOD(IMG1,%d) = 0, %d, MOD(IMG1,%d)) AS IMG1, IF(MOD(IMG2,%d) = 0, %d, MOD(IMG2,%d)) AS IMG2, IF(SUM(AnomalySUM), SUM(AnomalySUM),0.0)  As SUM FROM Collaborative_Start_Temp WHERE IMG2 > IMG1 GROUP BY IMG1,IMG2 ORDER BY SUM;",MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE);
		snprintf(sqlbuffer,BIGBUFFER,"CREATE TEMPORARY TABLE Collaborative_Result_Temp AS select IF(MOD(IMG1,%d) = 0, %d, MOD(IMG1,%d)) AS IMG1, IF(MOD(IMG2,%d) = 0, %d, MOD(IMG2,%d)) AS IMG2, IF(SUM(AnomalySUM), SUM(AnomalySUM),0.0)  As SUM FROM Collaborative_Start_Temp GROUP BY IMG1,IMG2;",MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE,MODVALUE);
	
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);	
	res = mysql_use_result(conn);


}

void getTabledata(MYSQL *connread, FILE *outfile, char *table, int time)
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
	int **connections;
	int image_count,x,y;

	//Create Array - Find how many images we have to create array
	snprintf(sqlbuffer, buffersize,"select MAX(IMG1) FROM %s ",table);
		
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);	
	
	if( (row = mysql_fetch_row(res)) != NULL)
	{
		image_count = atoi(row[0]);
	}
	else { printf("Error getting count of images.\n"); exit(1); }
	while( (row = mysql_fetch_row(res)) != NULL);
	
	//image_count = 400;
	printf("Creating matrix for %d images.\n", image_count);
	
	connections = (int **) malloc((image_count+1) * sizeof(int *));
	if(NULL == connections){free(connections); printf("Memory allocation failed.\n"); exit(2);}

	for(x = 0; x <= image_count; x++)
	{
	    connections[x] = (int *) malloc((image_count+1) * sizeof(int));
	    if(NULL == connections[x]){free(connections[x]); printf("Memory allocation failed.\n"); exit(2);}
	}   
	   
	for (x=0; x<=image_count; x++) {
		for(y=0; y<=image_count; y++)
		{	
			//fprintf(stderr,"%d x %d\n",x,y);
			connections[x][y] = 0;
		}
	}
	
	
	//Fill Array
	printf("Populating adjacency matrix.\n");
	snprintf(sqlbuffer, buffersize,"select * FROM %s ",table);
	
	if (mysql_query(connread,sqlbuffer) != 0)
		mysql_print_error(connread);
	res = mysql_use_result(connread);
	
	while( (row = mysql_fetch_row(res)) != NULL)
	{
			image_one = atoi(row[0]);
			image_two = atoi(row[1]);
			collaboration_num = atof(row[2]);
			if(collaboration_num > 0 && image_one != image_two)
				connections[image_one][image_two] = 1;
	}
	
	for (x=1; x<=image_count; x++) {
		printf("%d :", x);
		for(y=1; y<=image_count; y++)
		{
			//fprintf(stderr,"%d x %d\n",x,y);
			printf("%d, ",connections[x][y]);
		}
		printf("\n");
	}
	
	BFS(connections, image_count);
	
	
}

#define WHITE 0
#define GRAY 1
#define BLACK 2

void BFS(int **connections, int image_count)
{

	int v_start,v,u,x,y, distance;
	int *colors;
	colors = (int *) malloc( (image_count+1) * sizeof(int));
	if(colors == NULL) { fprintf(stderr,"Malloc Error\n"); exit(2); }
	
	for(v_start = 1; v_start <= image_count; v_start++)
	{
		y = 0;
		for(x = 1; x<=image_count; x++)
		{
			if(connections[v_start][x] > 0) y++;
		}
		if(y > 0)
		{
			for(v = 1; v <= image_count; v++)
				colors[v] = WHITE;
			
			distance = 0;
			v = v_start;
			colors[v] = GRAY;
			cleanQueue();
			
			enqueue(v);
			while(!QisEmpty())
			{
				if(dequeue(&u) == -1) {printf("Error with dequeue!\n"); exit(2); }
				
				//fprintf(stderr, "Dequeue %d\n", u);
				for(x=1; x<= image_count; x++)
				{
					if(connections[u][x] > 0)
					{
						v = x;
						if(colors[v] == WHITE)
						{
							colors[v] = GRAY;
							enqueue(v);
						}				
					}
				}//for x	
				colors[u] = BLACK;
				distance++;
				printf("%d ",u);
					
			}
			printf("\t Distance:%d\n",distance);
		}
		else 
		{
			printf("V%d has no connections.\n", v_start);
		}
	}//v_count
	
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
	char filename[255];
	char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
	int query_number;
	int c;
	MYSQL *conn = NULL;
    //Setup for MySQL Init File
    my_init();
	Qhead = NULL;
	Qtail = NULL;
	
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
		case 'Q' :
		   	strncpy(tempstring,optarg,19);
			query_number = atoi(tempstring);
			if(query_number <= 0) { fprintf(stdout,"Bad query value entered.\n"); exit(1); }
		  break;
	    }//switch      
     
    }//while
    
    //Move to end of Argument List
    argc -= optind; 
    argv += optind;
	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
	
	FILE *outgraph;
	snprintf(filename,255,"graph-%05d.dat",query_number);
	if( (outgraph= fopen(filename, "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n",filename); }

	fprintf(outgraph,"Graph\n{\n");
	
	conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	if(conn == NULL) { fprintf(stderr,"[Main] Error opening MySQL Connection.\n"); exit(1); }
	initTables(conn, query_number);
	getTabledata(conn, outgraph, "Collaborative_Result_Temp", query_number);
	fprintf(outgraph,"\n}\n");
	
	fclose(outgraph);
	mysql_close(conn);
    return(0);
}


