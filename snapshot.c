#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <sys/ioctl.h>
#include <mysql/mysql.h>
//#include <mysql/my_sys.h>
#include <getopt.h>
#include<linux/msdos_fs.h>
#include "hashFunctions.h"
#include "mysql.h"

#define BIGBUFFER 10000
#define sqlbufsize 255

#define db_name "SNAPSHOT"
#define DRIVE_TABLENAME "image_snapshot_table"
#define IMAGE_LIST_TABLENAME "image_list_table"
#define CASE_TABLENAME "case_table"
#define NCD_TABLENAME "NCD_table"


struct dirDFS {
  char dirname[PATH_MAX];
  struct dirDFS *next;  
};

//declare
struct dirDFS *createdirlist(char *);
void listfiles(struct dirDFS *, FILE *, MYSQL *, int);
int startImage(MYSQL *, char *, char *, char *, char *);/*Returns New Image Key*/
void initTables(MYSQL *);


//Flags
int GLOB_SQL = 1; //Output to SQL Database - Default
int GLOB_PRINT_FLAG = 0; //Instead of sending to SQL - Send output to file
int GLOB_FILE_FLAG = 0; //Instead of sending to SQL - Send output to file
int GLOB_HASH_FLAG = 1; //Hash Files - Default is Yes
int GLOB_INIT_FLAG = 0; //Initialize Tables - Default NO
int GLOB_IGNOREINFO_FLAG = 0; //Do not ask for Image Information


struct dirDFS *createdirlist(char *thedir)
{
	DIR	*dp; //DIR pointer
	struct dirent *dirp; //Dir struct pointer
	char fullname[PATH_MAX];

	struct stat statbuf; //stat for check dir
	char cursor[4]={'/','-','\\','|'};
	struct dirDFS *current;
	struct dirDFS *sub_current;
	struct dirDFS *head;
	struct dirDFS *newdfs;
	short int spot;
	
	current = malloc(sizeof(struct dirDFS));
	strncpy(current->dirname,thedir,PATH_MAX);
	current->next = NULL; //END 
	head = current;
	printf("Creating directory tree ");
	do {
	  //Open directory
	  if((dp = opendir( current->dirname)) == NULL)
	    {fprintf(stderr," \b \nError: Can't open dir %s\n",current->dirname); current = current->next; continue;}
	  sub_current = current;
	  //Loop Until  end
	  while ((dirp = readdir(dp)) !=NULL)
	  {	
	          if( strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 ) continue; //ignore . ..
		  if( *(current->dirname + strlen( current->dirname) - 1) == '/') *(current->dirname + strlen( current->dirname) - 1) = '\0'; //Take Leading / off
		  strncpy(fullname, current->dirname,PATH_MAX); 
		  strcat(fullname,"/");
		  strcat(fullname,dirp->d_name);
		  if(lstat(fullname, &statbuf) < 0)
		    {fprintf(stderr,"can't use lstat on %s\n",fullname); }
		  //If Directory Insert Directory into Link List
		  if(S_ISDIR(statbuf.st_mode) == 1) 
		  {
		    if(  (newdfs = malloc(sizeof(struct dirDFS))) == NULL ) { fprintf(stderr,"MALLOC Error\n"); exit(1);}
		    strncpy(newdfs->dirname, fullname, PATH_MAX);
		    newdfs->next = sub_current->next;
		    sub_current->next = newdfs;
		    sub_current = sub_current->next;
		  }
	  }
	  if( closedir(dp) == -1 )  { fprintf(stderr,"Error closing current dir"); exit(1);}
	  printf("%c\b", cursor[spot]); //Found this idea someplace
	  fflush(stdout);
	  spot = (spot+1) % 4;

	  current = current->next;
	} 
	while( current != NULL);
	printf(" \b ");fflush(stdout);
	printf("\n");
	return head;
}

#define INSERT_INPUT "(image, directory, filename, file_type, file_perm,\
num_links, unix_uid, unix_gid, file_size, timestamp_modify,\
timestamp_access,timestamp_create, DOS_attrib, file_hash)"

void listfiles(struct dirDFS *currentdir, FILE *fileoutput, MYSQL *conn, int key)
{
  	DIR	*dp; //DIR pointer
	struct dirent *dirp; //Dir struct pointer
	struct stat statbuf; //stat for check dir
	char fullname[PATH_MAX];
	struct passwd *pwptr; //pw->pw_name
	struct group *grpptr; //grp->gr_name
	int fd;
	__u32 attrib;
	struct __fat_dirent dosdir[2];
	char sqlbuffer[BIGBUFFER];
	char tempbuffer[BIGBUFFER];
	short int spot;
	char cursor[4]={'/','-','\\','|'};
	
	printf("Insert items.\n");
  	while(currentdir != NULL)
	{
		//printf("%s\n",currentdir->dirname);
		if((dp = opendir( currentdir->dirname)) == NULL)
		    {fprintf(stderr," \b \nError: Can't open dir %s\n",currentdir->dirname); currentdir = currentdir->next; continue;}
	
		while ((dirp = readdir(dp)) !=NULL)
		{	
		    if( strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 ) continue; //ignore . ..
		    snprintf(fullname,PATH_MAX,"%s/%s",currentdir->dirname,dirp->d_name);
		    if(lstat(fullname, &statbuf) < 0)
		      {fprintf(stderr,"can't use lstat on %s\n",fullname); }


		      snprintf(sqlbuffer,BIGBUFFER,"INSERT INTO %s %s VALUES ( %d, ", DRIVE_TABLENAME, INSERT_INPUT, key);
		      snprintf(tempbuffer,BIGBUFFER," \"%s\", \"%s\", ",currentdir->dirname, dirp->d_name);
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      
		      //File Type
		      if(S_ISREG(statbuf.st_mode)) snprintf(tempbuffer,BIGBUFFER,"\"r");
		      else if (S_ISDIR(statbuf.st_mode)) snprintf(tempbuffer,BIGBUFFER,"\"d");
		      else if (S_ISLNK(statbuf.st_mode)) snprintf(tempbuffer,BIGBUFFER,"\"s");
		      else if (S_ISCHR(statbuf.st_mode)) snprintf(tempbuffer,BIGBUFFER,"\"c");
		      else if (S_ISBLK(statbuf.st_mode)) snprintf(tempbuffer,BIGBUFFER,"\"b");
		      else if (S_ISSOCK(statbuf.st_mode)) snprintf(tempbuffer,BIGBUFFER,"\"S");
		      else if (S_ISFIFO(statbuf.st_mode)) snprintf(tempbuffer,BIGBUFFER,"\"f");
		      else snprintf(tempbuffer,BIGBUFFER,"*");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
			  
		      //file perms
		      snprintf(tempbuffer,BIGBUFFER,"\", \"");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      //usr
		      snprintf(tempbuffer,BIGBUFFER,"%s",(statbuf.st_mode & S_IRUSR)?"r":"-");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      snprintf(tempbuffer,BIGBUFFER,"%s",(statbuf.st_mode & S_IWUSR)?"w":"-");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      snprintf(tempbuffer,BIGBUFFER,"%s",
		      (statbuf.st_mode & S_IXUSR)?
		      ((statbuf.st_mode & S_ISUID)?"s":"x")
		      :((statbuf.st_mode & S_ISUID)?"S":"-"));
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      //group
		     snprintf(tempbuffer,BIGBUFFER,"%s",(statbuf.st_mode & S_IRGRP)?"r":"-");
		     strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      snprintf(tempbuffer,BIGBUFFER,"%s",(statbuf.st_mode & S_IWGRP)?"w":"-");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      snprintf(tempbuffer,BIGBUFFER,"%s",
		      (statbuf.st_mode & S_IXGRP)?
		      ((statbuf.st_mode & S_ISGID)?"s":"x")
		      :((statbuf.st_mode & S_ISGID)?"S":"-"));
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      //world-other
		      snprintf(tempbuffer,BIGBUFFER,"%s",(statbuf.st_mode & S_IROTH)?"r":"-");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      snprintf(tempbuffer,BIGBUFFER,"%s",(statbuf.st_mode & S_IWOTH)?"w":"-");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      snprintf(tempbuffer,BIGBUFFER,"%s",
		      (statbuf.st_mode & S_IXOTH)?
		      ((statbuf.st_mode & S_ISVTX)?"s":"x")
		      :((statbuf.st_mode & S_ISVTX)?"S":"-"));
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      snprintf(tempbuffer,BIGBUFFER,"\", ");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      
		      //Number of Links
		     snprintf(tempbuffer,BIGBUFFER,"%i, ", statbuf.st_nlink);
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      //if valid UID/GID print name if not print uid/gid
		      if(((pwptr = getpwuid(statbuf.st_uid)) != NULL) 
		      && ((grpptr = getgrgid(statbuf.st_gid)) != NULL))
		      snprintf(tempbuffer,BIGBUFFER,"\"%s\", \"%s\", ", pwptr->pw_name, grpptr->gr_name);
		      else snprintf(tempbuffer,BIGBUFFER,"%i, %i,  ",statbuf.st_uid, statbuf.st_gid);
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      //File Size
		      snprintf(tempbuffer,BIGBUFFER,"%i, ",statbuf.st_size);
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      
		      //File Times M A C
			  strftime(tempbuffer, BIGBUFFER,"\"%Y-%m-%d %H:%M:%S\", ",gmtime(&statbuf.st_mtime));
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
			  strftime(tempbuffer, BIGBUFFER,"\"%Y-%m-%d %H:%M:%S\", ",gmtime(&statbuf.st_atime));
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
			  strftime(tempbuffer, BIGBUFFER,"\"%Y-%m-%d %H:%M:%S\", ",gmtime(&statbuf.st_ctime));
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);

		      //Attempt to get MSDOS Attributes
		      fd = open(fullname, O_RDONLY);
		      
		      if( ioctl(fd,FAT_IOCTL_GET_ATTRIBUTES, &attrib) != -1 )
		      {
			snprintf(tempbuffer,BIGBUFFER,"\"%s%s%s%s%s%s\", ", ((attrib & ATTR_RO)?"r":"-"), ((attrib & ATTR_SYS)?"s":"-"),((attrib & ATTR_VOLUME)?"v":"-"),((attrib & ATTR_DIR)?"d":"-"),((attrib & ATTR_ARCH)?"a":"-"),((attrib & ATTR_HIDDEN)?"h":"-"));
		      }
		      else
			snprintf(tempbuffer,BIGBUFFER,"\"------\", ");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      
		      //Hash File
		      if(S_ISREG(statbuf.st_mode) && (GLOB_HASH_FLAG == 1))
		      {
			 if( hashfile(fullname, tempbuffer) == -1 ) 
			    {
			      fprintf(stderr,"Error Hashing: %s\n",fullname);
			      snprintf(tempbuffer,BIGBUFFER,"\" \"");
			    }
		      }
		      else snprintf(tempbuffer,BIGBUFFER,"\" \"");
		      strncat(sqlbuffer,tempbuffer,BIGBUFFER);
		      /*
		      //Short Name MSDOS
		      if( ioctl(fd, VFAT_IOCTL_READDIR_BOTH, &dosdir) != -1 )
		      {
			printf("Short %s %s ",dosdir[0].d_name, dosdir[1].d_name);
		      }
		      else 
			printf(" \"None\"");
		      */ 
		      
		      close(fd);
		      
		      //The End
		      strncat(sqlbuffer,");",BIGBUFFER);
		      
		      if(GLOB_PRINT_FLAG)
		      {
			printf("%s\n",sqlbuffer);
			
		      }
		      else if (GLOB_FILE_FLAG )
		      {
			fprintf(fileoutput,"%s\n",sqlbuffer);
		      }
		      else if (GLOB_SQL)
		      {
			if (mysql_query(conn,sqlbuffer) != 0)
			  mysql_print_error(conn);
		      }
		      else exit(1); //Going No Where
		      
		      

		}
	    if( closedir(dp) == -1 )  { fprintf(stderr,"Error closing current dir"); exit(1);}
	    printf("%c\b", cursor[spot]); //Found this idea someplace
	    fflush(stdout);
	    spot = (spot+1) % 4;
	    currentdir = currentdir->next;	  
	}
	printf(" \b ");fflush(stdout);
	printf("\n");
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
	else if ((case_num = atoi(case_number)) == 0)
	{
			//New Case - Get Name - Create Case - Get Last Value
			if(GLOB_IGNOREINFO_FLAG == 0)
			{
				printf("Enter the case name:");
				fgets(case_name, sqlbufsize-2,stdin);
			}
			else 
			{ 
				time_t curr_time = time(NULL);
				strftime(case_name,sqlbufsize-2,"Default %c",localtime(&curr_time));
				printf("Using default case name: %s\n",case_name);
				//snprintf(case_name,sqlbufsize-2,"Default\n");
			}
	
			snprintf(sqlbuffer, BIGBUFFER, "INSERT %s (case_name) VALUES (\"%s\");", CASE_TABLENAME, strtok(case_name, "\n"));
			fprintf(stderr,"Test %s\n",sqlbuffer);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
		
			snprintf(sqlbuffer, BIGBUFFER,"select MAX(case_number) FROM %s;",CASE_TABLENAME);
			//Check
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
			res = mysql_use_result(conn);
			if( (row = mysql_fetch_row(res)) != NULL)
			{
				case_num = atoi(row[0]);
				if (case_num > 0) {check = 1;}
				else printf("Error entering new case\n");
				while( (row = mysql_fetch_row(res)) != NULL); //goto end	
			}
			else printf("Error entering new case\n");
	}
	
	while(check == 0)
	{
		printf("\nPlease enter case number [L][N]][?]: ");
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
		else if ( !strncasecmp(tempstring, "N", 1) )
		{
			//New Case - Get Name - Create Case - Get Last Value
			printf("Enter the case name:");
			fgets(case_name, sqlbufsize-2,stdin);
	
			snprintf(sqlbuffer, BIGBUFFER, "INSERT %s (case_name) VALUES (\"%s\");", CASE_TABLENAME, strtok(case_name, "\n"));
			fprintf(stderr,"Test %s\n",sqlbuffer);
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
		
			snprintf(sqlbuffer, BIGBUFFER,"select MAX(case_number) FROM %s;",CASE_TABLENAME);
			//Check
			if (mysql_query(conn,sqlbuffer) != 0)
				mysql_print_error(conn);
			res = mysql_use_result(conn);
			if( (row = mysql_fetch_row(res)) != NULL)
			{
				case_num = atoi(row[0]);
				if (case_num > 0) {check = 1;}
				else printf("Error entering new case\n");
				while( (row = mysql_fetch_row(res)) != NULL); //goto end	
			}
			else printf("Error entering new case\n");
		}
		else if (!strncasecmp(tempstring, "?", 1) )
		{
			printf("Help:\nEnter number of a case. \nEnter [L] for List of current cases. \nEnter [N] to create a new case\n"); 		
		}	
	}//While
	
	return case_num;
}


int startImage(MYSQL *conn, char *filepath, char* investigator, char *case_name, char *image_name)
{
	if( conn == NULL) return -1;
	
	char sqlbuffer[BIGBUFFER];
	MYSQL_RES *res_set;
	MYSQL_ROW row;
	int key;
	int case_number;

	//Get Optional Information
	if(GLOB_IGNOREINFO_FLAG == 0)
	{
		if(investigator == NULL) 
		{
			printf("Enter the investigator name:");
			investigator = malloc(sizeof(char) * sqlbufsize);
			fgets(investigator, sqlbufsize-1 ,stdin);
		}
		case_number = pickCase(conn, case_name);
		fprintf(stderr,"Using Case Number: %d\n",case_number);
		if(image_name == NULL) 
		{
			printf("Enter the image name:");
			image_name = malloc(sizeof(char)*sqlbufsize);
			fgets(image_name, sqlbufsize-1,stdin);
		}
	
	}
	else
	{
		if(investigator == NULL) { investigator = malloc(sizeof(char) * sqlbufsize); strcpy(investigator,"None"); }
		case_number = pickCase(conn, case_name);
		if(image_name == NULL) { image_name = malloc(sizeof(char)*sqlbufsize); strcpy(image_name,"None"); }

	}
	
	//Send Query to Insert a new Row
	//Todo - Add more File Systeme Information
	snprintf(sqlbuffer, BIGBUFFER, "INSERT %s (image_start, image_casenumber, image_investigator_name, image_imagename) VALUES (\"%s\",\"%d\",\"%s\",\"%s\");", IMAGE_LIST_TABLENAME, filepath,case_number, strtok(investigator, "\n"), strtok(image_name, "\n"));
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	
	//Now Find the last image key to use to use as a foreign key when collection data
	snprintf(sqlbuffer, BIGBUFFER,"SELECT MAX(image) FROM %s;",IMAGE_LIST_TABLENAME);
	if (mysql_query (conn, sqlbuffer) != 0)
		mysql_print_error(conn);
	else
	{
		if ( (res_set = mysql_store_result (conn)) == NULL)
			mysql_print_error(conn);
		else
		{
			if((row = mysql_fetch_row (res_set)) == NULL)
				{ fprintf(stderr,"Error reading Row.\n"); exit(1); }
			//Row[0] should hold key
			key = atoi(row[0]);
		
			mysql_free_result (res_set);
		}			
	}
	if(key == 0) {fprintf(stderr,"No Valid Key Returned.\n"); exit(1); }
	return key;

}

void initTables(MYSQL *conn)
{
	if(conn == NULL) exit(1);
	
	char sqlbuffer[BIGBUFFER];
	
	fprintf(stderr,"Intializing Tables in Database\n");
	fprintf(stderr,"\tDropping table %s\n",DRIVE_TABLENAME);
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;",DRIVE_TABLENAME);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	fprintf(stderr,"\tDropping table %s\n",IMAGE_LIST_TABLENAME);
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;",IMAGE_LIST_TABLENAME);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	fprintf(stderr,"\tDropping table %s\n",CASE_TABLENAME);
	snprintf(sqlbuffer, BIGBUFFER,"DROP table %s;",CASE_TABLENAME);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
		
	fprintf(stderr,"\tCreating table %s\n",DRIVE_TABLENAME);
	snprintf(sqlbuffer,BIGBUFFER,"%s",TABLE1);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	fprintf(stderr,"\tCreating table %s\n",IMAGE_LIST_TABLENAME);
	snprintf(sqlbuffer,BIGBUFFER,"%s",TABLE2);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);
	fprintf(stderr,"\tCreating table %s\n",CASE_TABLENAME);
	snprintf(sqlbuffer,BIGBUFFER,"%s",CREATE_CASETABLE);
	if (mysql_query(conn,sqlbuffer) != 0)
		mysql_print_error(conn);

}
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
  {"nohash", no_argument, NULL, 'n'},
  {"noinformation", no_argument, NULL, 'N'},
  {"investigator", required_argument, NULL, 'i'},
  {"case-number", required_argument, NULL, 'c'},
  {"image-name", required_argument, NULL, 'm'},
  {"intialize", no_argument, NULL, 'I'},
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
    printf("\t-S, --screen\t Dumps SQL INSERT commands to STDOUT.\n");
    printf("\t-N, --noinformation\t Does not ask for image information.\n");
	printf("\t-n, --nohash\t Does not perform SHA1 Hashing for files.\n");
	printf("\t-i, --investigator\t Investigator name\n");
	printf("\t-c, --case-number\t Case Number\n");
	printf("\t-m, --iMage-name\t Image Name\n");

    printf("\t-h, --help\t This help page.\n");
  
}
#define PASS_SIZE 100
int main(int argc, char *argv[] )
{
  
    struct dirDFS *directory;
    char *host_name = NULL;
    char *user_name = NULL;
    char *password = NULL;
    char passbuffer[PASS_SIZE]; // Just in case we want a NULL Password parm - create a buffer to point to
    unsigned int port_num = 0;
    char *socket_name = NULL;
    int input,option_index,key;
    char scandir[PATH_MAX];
    FILE *fileoutput = NULL;
    char OUTPUT_PATH[PATH_MAX];
    MYSQL *conn = NULL;
	char *investigator = NULL;
	char *case_number = NULL;
	char *image_name = NULL;
    
    //Setup For OpenSSL Hash
    OpenSSL_add_all_digests();
    
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
	  case_number = optarg;
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
	case 'n' :
	  GLOB_HASH_FLAG = 0;
	  break;
	case 'I' :
	  GLOB_INIT_FLAG = 1;
	  break;
	case 'N' :
	  GLOB_IGNOREINFO_FLAG = 1;
	  break;
	case 'S' :
	  GLOB_PRINT_FLAG = 1;
	  GLOB_SQL = 0;
	  break;
	case 'F' :
	  GLOB_FILE_FLAG = 1;
	  GLOB_SQL = 0;
	  snprintf(OUTPUT_PATH,PATH_MAX,"%s\0",optarg);
	  break;
		  
      }//switch      
     
    }//while
    
    //Move to end of Argument List
    argc -= optind; 
    argv += optind;

	if(password == NULL) password = DEFAULT_PASS;
	if(user_name == NULL) user_name = DEFAULT_USER;
    
    //Used CWD if nothing else specified
    if(argc == 0)
    {
     if(getcwd(scandir,PATH_MAX) == NULL)
     {fprintf(stderr,"Failed to get Current Working directory or directory from command line.\n"); exit(1);}
    }
    else
    {
     strncpy(scandir,argv[0],PATH_MAX); 
    }
    
	key = 0;
    //Setup Output
    if(GLOB_FILE_FLAG == 1)
    {
      //Open Output File
       if( (fileoutput = fopen(OUTPUT_PATH, "w")) == NULL) { fprintf(stderr,"Error opening output file!\n"); exit(1); }
    }
    else if(GLOB_SQL == 1)
    {
     //Setup SQL  
     conn = mysql_connect(host_name,user_name,password,db_name, port_num, socket_name, 0);
	 if(GLOB_INIT_FLAG == 1) initTables(conn);
	 else
	 key = startImage(conn, scandir, investigator, case_number, image_name);
	 
    }
    
    if(GLOB_INIT_FLAG == 0)
    {
    //Make List of Directories
    directory = createdirlist(scandir);
     //Output files into SQL or Screen or File
	listfiles(directory, fileoutput, conn, key);
    }

    //Close Output
    if(GLOB_FILE_FLAG == 1)
    {
      //Close Output File
       fclose(fileoutput);
    }
    else if(GLOB_SQL == 1)
    {
     //Sql End
     if (mysql_query(conn,"COMMIT;") != 0)
	mysql_print_error(conn);
      mysql_close(conn);
    }
    
    return(0);
}


