/*



Changes:
2/9
Added new (predictable)  random string function to replace the use of /dev/urandom - provides UPPER and LOWER bound selection
Created Srand function and supporting options to replace the use of the standard srand fuction - in order to recreate results at will
Change Modify Offset to use randomString function - changed upper/lower bound to 1,0 (creating a string of zeros) //As suggested by professor
	This still leaves open the option to have a randomly sized modification
Added a command line option for Consipiracy Size Group
	

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <getopt.h>

#define VERSION "3"
//================================================

// these should really eventually be args

// people
#define PEOPLE 20

// groups
#define ORGS 3

// parameters defining number of members in a group
#define MINGROUPSIZE 5
#define MAXGROUPSIZE 10
int CONSPIRACYSIZE = 2;

// parameters defining number of files shared by group members
#define NUMSYSTEMFILES 10
#define NUMCONSPIRACYFILES 4
#define MINGROUPFILES 3
#define MAXGROUPFILES 5

// how long to simulate 
#define DEFAULT_SIMDURATION 2

//Random and File Arguments
#define RANDOM_SOURCE "/dev/urandom"
#define RANDOM_SIZE 65536
#define BLOCK 8192

//File Change Arguments
#define PERCENT_CHANGE 10

#define DEFAULT_SRANDNAME "SEEDS"


//Random Bytes to use during file creation
#define UPPERBOUND 255
#define LOWERBOUND 0

//Modify chunk size bounds
#define MOD_UPPER 8192
#define MOD_LOWER 8192

struct Srand_Struct {
	char filename[PATH_MAX];
	int readwrite;
	FILE *file;
};
typedef struct Srand_Struct Srand_Struct;
Srand_Struct SrandStruct;


//================================================
// HEADER
//================================================
struct FileData {
  char name[255];
};
typedef struct FileData FileData;

//================================================
struct Person {
  int person_id;
  int num_groups;
  struct Group** groups;
  char dir[255];
  // To do: add other fields as needed
};
typedef struct Person Person;

//================================================
struct Group {
  int group_id;
  int num_members;
  int num_files;
  Person** members;
  // To do: add other fields as needed
};
typedef struct Group Group;

int doPickFileByMemory(Person*, int*, char *);
int doPickFileByDirSearch(char *,  char *);
int doModify(char *, char *, int, ssize_t);
int doModifyRandOffset(char *, char *, ssize_t);


int Srand(Srand_Struct *S)
{

	long unsigned int seed;
	size_t io;
	
	if(S->file == NULL) 
		if(S->readwrite == 0) { if( (S->file = fopen(S->filename, "wb")) == NULL) {printf("Error opening %s\n", S->filename); exit(1);}}
		else { if( (S->file = fopen(S->filename, "rb")) == NULL) {printf("Error opening %s\n", S->filename); exit(1);}}
		
	if(S->readwrite == 0) {
		//write new and seed
		seed = (long unsigned int) (time(NULL) & rand());
		io = fwrite(&seed, sizeof(long unsigned int), 1, S->file);
		if(io == 0) {printf("Failed to write to file %s\n",S->filename); exit(1); }
		srand(seed);
	}
	else
	{
		io = fread(&seed, sizeof(long unsigned int), 1, S->file);
		if(io == 0) {printf("Failed to read from file %s\n",S->filename); exit(1); }
		srand(seed);	
	}
	printf("Intialize SRAND with: %lu\n",seed);
	
	return 0;
} // SRand

//================================================
// SOURCE 
//================================================
Person* makePerson(int i, int CREATENEW) {
  printf("*** making person %d\n",i);
  Person* p = (Person*) malloc(sizeof(struct Person));
  p->person_id=i;
  sprintf(p->dir,"person-%d",i);

  if(CREATENEW == 1)
  {
	  // make a directory
	  char cmd[255];
	  sprintf(cmd, "mkdir %s", p->dir);
	  system(cmd);
  }

  // To do: initialize other fields as needed
  return p;
}


int randomString( char *rstring, int size, int upper, int lower)
{
	int count;
	for(count = 0; count < size; count++)
    {
        rstring[count] = (char)((rand() % (upper-lower)) + lower);
    }
	return count;
}

//================================================
Group* makeGroup(int i, int gsize, int files, Person** people, int CREATENEW) {
  int j, k, f, c;
  int fdin,fdout;
  char buffer[BLOCK+1];
  char startfilename[PATH_MAX+1];
  char newfilename[PATH_MAX+1];
  ssize_t readbytes, writebytes, totalbytes;
  printf("*** making group %d\n",i);
  Group* g=(Group*) malloc(sizeof(struct Group));
  g->group_id=i;
  
  printf("group %d will have size %d\n",i, gsize);
  g->num_members=gsize;
  
  g->members = (Person**)malloc(g->num_members * sizeof(Person*));
 c=0;//count j= random person
 for(j = 0; j < PEOPLE; j++)
	if((rand() % (PEOPLE-j))<gsize)
	{
		g->members[c]=people[j];
		printf("added person %d as the %dth member of group %d\n", people[j]->person_id, c, i);
		c++;
		gsize--;
	}

  printf("the members of group %d are sharing %d files \n",i,files);

  g->num_files = files;
  
  if(CREATENEW == 1)
  {
	  //1.  Create Temp File in member 0's folder
	  //2. Copy Temp file into other user directories
	  if(g->num_members == 0) { printf("No users to add files to.\n"); return g;}
	  if(g->num_files == 0) { printf("No files to add.\n"); return g;}
	  for (f=0; f<files; f++) 
		{ 
			//Make Random File
			totalbytes=0;
			//if( (fdin = open(RANDOM_SOURCE, O_RDONLY)) < 0) { fprintf(stderr,"Can't open RANDOM file. %s\n",RANDOM_SOURCE); }
			snprintf(startfilename, PATH_MAX,"%s/group%d-file%d",g->members[0]->dir,i,f);
			if( (fdout = creat(startfilename, S_IRUSR | S_IWUSR )) < 0) { fprintf(stderr,"Can't create file. %s\n",startfilename); }
			//while( ((readbytes = read(fdin, buffer, BLOCK)) > 0 ) && totalbytes < RANDOM_SIZE )
			while( totalbytes < RANDOM_SIZE )
			{
				readbytes = randomString(buffer, BLOCK, UPPERBOUND, LOWERBOUND);
				if( (writebytes=write(fdout, buffer, BLOCK)) != readbytes) {fprintf(stderr,"Error writting to output file %s (%d/%d).\n", startfilename, readbytes, writebytes); }	
				totalbytes += writebytes;
			}
			if(readbytes<0) {fprintf(stderr,"Error reading data. %s\n",RANDOM_SOURCE);}
			close(fdin);
			close(fdout);
			
			//Copy Files
			if( (fdin = open(startfilename, O_RDONLY)) < 0) { fprintf(stderr,"Can't open file. %s\n",startfilename); }
			for(j = 1; j < g->num_members; j++)
			{
				snprintf(newfilename, PATH_MAX,"%s/group%d-file%d",g->members[j]->dir,i,f);
				if( (fdout = creat(newfilename, S_IRUSR | S_IWUSR )) < 0) { fprintf(stderr,"Can't create file. %s\n",newfilename); }
				while( (readbytes = read(fdin, buffer, BLOCK)) > 0 )
					if( (write(fdout, buffer, BLOCK)) != readbytes) {fprintf(stderr,"Error writting to output file %s.\n", newfilename); }	
				if(readbytes<0) {fprintf(stderr,"Error reading data. %s\n",startfilename);}
				if(lseek(fdin, 0, SEEK_SET) == -1) {fprintf(stderr,"Error seeking to beginning of file %s.\n",startfilename);}
				close(fdout);
			}
	   } //Create file for loop
   }

  // To do: initialize other fields as needed
  return g;
}

//================================================
void informPeopleOfTheirGroups(Person** people, Group** orgs) {
  int i, j, k, ct;
  // tell people about their groups
  for (i=0; i<PEOPLE; i++) {
    Person * p = people[i];
    ct = 0;

    for (j=0; j<ORGS; j++) {
      Group * g = orgs[j];
      for (k=0;k<g->num_members;k++) {
	if (g->members[k]==p) {
	  ct++;
	}
      }
    }
    p->num_groups = ct;
    p->groups = (Group**)malloc(ct * sizeof(Group*));
    ct=0;
    for (j=0; j<ORGS; j++) {
      Group * g = orgs[j];
      for (k=0;k<g->num_members;k++) {
	if (g->members[k]==p) {
	  p->groups[ct]=g;
	  ct++;
	}
      }
    }
  }
}

int doPickFileByDirSearch(char *CWD,  char *filename)
{
	DIR	*dp; //DIR pointer
	struct dirent *dirp; //Dir struct pointer
	char fullname[PATH_MAX];
	int fcount = 0;
	int randnum, count;
	struct stat statbuf; //stat for check dir

	if((dp = opendir(CWD)) == NULL)
	    {fprintf(stderr," \b \nError: Can't open dir %s\n",CWD); return 1;}
	while ((dirp = readdir(dp)) !=NULL)
	{	
		if( strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 ) continue; //ignore . ..
		if( *(CWD + strlen(CWD) - 1) == '/') *(CWD + strlen(CWD) - 1) = '\0';
		snprintf(fullname, PATH_MAX,"%s/%s",CWD,dirp->d_name); 
		if(lstat(fullname, &statbuf) < 0)
		{fprintf(stderr,"can't use lstat on %s\n",fullname); }
		if(S_ISREG(statbuf.st_mode)) fcount++;
	}//while get dcount
	
	if(fcount == 0) {printf("No Files found in %s\n",CWD); return 2;}
	Srand (&SrandStruct);
    randnum = (rand() % fcount) + 1;
	rewinddir(dp);
	count = 0;
	
	while ((dirp = readdir(dp)) !=NULL)
	{	
		if( strcmp(dirp->d_name, ".") == 0 || strcmp(dirp->d_name, "..") == 0 ) continue; //ignore . ..
		if( *(CWD + strlen(CWD) - 1) == '/') *(CWD + strlen(CWD) - 1) = '\0';
		snprintf(fullname, PATH_MAX,"%s/%s",CWD,dirp->d_name); 
		if(lstat(fullname, &statbuf) < 0)
		{fprintf(stderr,"can't use lstat on %s\n",fullname); }
		if(S_ISREG(statbuf.st_mode)) count++;
		if(count == randnum) break;
	}
	if(dirp == NULL) { fprintf(stderr,"Error getting file %d.\n",randnum); return 1;}
	else {
		snprintf(filename, PATH_MAX,"%s",dirp->d_name); 
	}
	if( closedir(dp) == -1 )  { fprintf(stderr,"Error closing current dir"); exit(1);}
	return 0;

}

int doModify(char *userdir, char *filename, int chance, ssize_t granularity)
{
	int fdorig;
	int fdnew;
	int readbytes;
	int randombyte;
	char filebuffer[granularity+1];
	ssize_t countbytes = 0;
	ssize_t changebytes = 0;
	char fullfilename[PATH_MAX+1];
	char tmpfilename[PATH_MAX+1];
	snprintf(fullfilename, PATH_MAX,"%s/%s",userdir,filename); 	
	snprintf(tmpfilename, PATH_MAX,"%s/%s~",userdir,filename); 	
	

	if( (fdorig = open(fullfilename, O_RDONLY)) < 0) { fprintf(stderr,"Can't open file. %s\n",fullfilename); }
	if( (fdnew = creat(tmpfilename, S_IRUSR | S_IWUSR )) < 0) { fprintf(stderr,"Can't create file. %s\n",tmpfilename); }
	
	while( (readbytes = read(fdorig, filebuffer, granularity)) > 0 )
	{
		countbytes += readbytes;
		if ( (rand() % 100) < chance)
		{
			randombyte = (rand() % 255);
			memset(filebuffer, randombyte, granularity);
			changebytes += readbytes;
		}
		if( (write(fdnew, filebuffer, granularity)) != readbytes) {fprintf(stderr,"Error writting to output file.\n"); }	
	}
	if(readbytes<0) {fprintf(stderr,"Error reading data.\n");}
	
	close(fdorig);
	close(fdnew);
	
	//Remove original and rename new file
	if( remove(fullfilename) == -1 ) 
		{ fprintf(stderr,"Error removing original file.\n"); return 1;};
	if( rename(tmpfilename, fullfilename) == -1 ) 
		{ fprintf(stderr,"Error renaming file.\n"); return 1;};
	
	fprintf(stderr,"Modified %s [%d/%d]\n",fullfilename,changebytes,countbytes);

	return 0;
}

int doModifyRandOffset(char *userdir, char *filename, ssize_t modsize)
{
	int fd;
	int count;
	
	char filebuffer[modsize+1];
	off_t eofbytes = 0;
	off_t startoff = 0;
	
	Srand (&SrandStruct);
	char fullfilename[PATH_MAX+1];
	snprintf(fullfilename, PATH_MAX,"%s/%s",userdir,filename); 	
	
	if( (fd = open(fullfilename, O_RDWR)) < 0) { fprintf(stderr,"Can't open file. %s\n",fullfilename); }
	
	if ( (eofbytes = lseek(fd, 0, SEEK_END) ) == -1 ) {printf("Error seeking to end of file on %s.",filename); return -1; }
	if( eofbytes <= modsize ) { startoff = 0; modsize = eofbytes;}
	else
	{
		startoff = (rand() % (eofbytes-modsize));
	}
	
	if ( (lseek(fd, startoff, SEEK_SET)) == -1 ) {printf("Error seeking to random offset %d on %s.",startoff, filename); return -1; }	
	
	randomString(filebuffer, modsize, 2, 0);
	
	if( (write(fd, filebuffer, modsize)) != modsize) {fprintf(stderr,"Error writting to output file %s.\n",filename); }	
	close(fd);
	fprintf(stderr,"Modified %d bytes of %s at offset %d\n",modsize, fullfilename, startoff);
	return 0;
}

int outputSeed(int seed)
{
	FILE *seedout;
	if( (seedout= fopen("SEED", "w")) == NULL) { fprintf(stderr,"Can't open file. %s\n","SEED"); }
	fprintf(seedout,"%d",seed);
	fclose(seedout);
}

int doPickFileByMemory(Person* p, int* groupfiles, char *filename)
{
	int i,f, filenum,max = 0;
	int count=0;
	for(i = 0; i < p->num_groups;i++)
	{
		max += groupfiles[p->groups[i]->group_id];
	}

	Srand (&SrandStruct);
	filenum = rand() % max;
	
	for(i = 0; i < p->num_groups;i++)
		for(f = 0; f < groupfiles[p->groups[i]->group_id]; f++)
		{
			if(count >= filenum) 
			{
				snprintf(filename, PATH_MAX,"group%d-file%d",p->groups[i]->group_id,f);
				return 0;
			}
			else count += 1;
		}
	

	return -1;
}// doPickFilesByMemory


//================================================
void simPerson(Person* p, int *gf) {

short MAX = 1;
char filename[PATH_MAX+1];
int numbytes;
int upper = MOD_UPPER; // prevent div 0 error on compile

if(MOD_UPPER == MOD_LOWER) numbytes = MOD_UPPER;
else numbytes = (rand() % (upper - MOD_LOWER)) + MOD_LOWER;

  printf("*** simulating person %d\n",p->person_id);
	switch((rand() % MAX) + 1)
	    {
		case 1 :
			doPickFileByMemory(p, gf, filename);
			//doPickFileByDirSearch(p->dir,filename);
			//doModify(p->dir,filename,PERCENT_CHANGE,1);
			doModifyRandOffset(p->dir,filename,numbytes);
			break;
		default :
			printf("Case error\n");
		break;
	    }//switch
}

//================================================
void simGroup(Group* g) {
  printf("*** simulating group %d\n",g->group_id);
  // To do...
}

void printhelp(char *filename)
{
	printf("Work Group file Simulation Version %s.\n",VERSION);
	printf("Bilal Khan\tRichard Alcalde \n"); 
	printf("Usage: ./%s [OPTIONS]\n",filename);
	printf("\t-S, --srandomfile\t\tFilename of SRAND seed store.\n\t\t\t\t\tDefault %s\n", DEFAULT_SRANDNAME);	
	printf("\t-R, --srandomread\t\tInstead of creating new SRAND seeds, use values from file.\n\t\t\t\t\n");	
	printf("\t-s, --seed\t\tUse a previously created seed for intial group creation.\n\t\t\t\tExample --seed 1234567\n");	
	printf("\t-c, --create\tCreate files - required for first run.\n\t\t\t\t\n");
	printf("\t-d, --duration\tHow many iterations on this simulation.\n\t\t\t\tExample --duration 1\tDefault: %d\n", DEFAULT_SIMDURATION);
	printf("\t-C, --conspiracysize\tHow many people in the conspiract group.\tDefault: %d\n", CONSPIRACYSIZE);
	printf("\t-h, --help\t This help page.\n");
	
}
//This is used for Get Options
struct option long_options[] =
{
   {"help", no_argument, NULL, 'h'},
   {"create", no_argument, NULL, 'c'},
   {"duration", required_argument, NULL, 'd'},
   {"seed", required_argument, NULL, 's'},
   {"srandomfile", required_argument, NULL, 'S'},
   {"srandomread", no_argument, NULL, 'R'},
   {"conspiracysize", required_argument, NULL, 'C'},
   { 0, 0, 0, 0 }
}; //long options

//================================================
int main(int argc, char** argv) {
	int i, t;
	Person** people;
	Group** orgs;
	int * groupsizes;
	int * groupfiles;
	//Getopts
	char tempstring[PATH_MAX];
	int input, option_index;
	int SIMDURATION = DEFAULT_SIMDURATION;
	int SEED = 0;

	short CREATENEW = 0;
	strncpy(SrandStruct.filename, DEFAULT_SRANDNAME, PATH_MAX);
	SrandStruct.readwrite = 0;
  
	while((input = getopt_long(argc, argv, "hs:d:S:RcC:", long_options, &option_index)) != EOF )
	{
		switch(input)
		{
		case 'h' :
		  printhelp("company");
		  exit(1);
		break;
		case 'd' :
		strncpy(tempstring,optarg,19);
		SIMDURATION = atoi(tempstring);
		if(SIMDURATION < 0) { fprintf(stdout,"Bad value entered.\n"); exit(1); }
		break;
		case 's' :
		strncpy(tempstring,optarg,19);
		SEED = atoi(tempstring);
		if(SEED < 0) { fprintf(stdout,"Bad value entered.\n"); exit(1); }
		break;
		case 'R' :
		SrandStruct.readwrite = 1;
		break;
		case 'S' :
		strncpy(SrandStruct.filename, optarg, PATH_MAX);		
		break;		
		case 'c':
		CREATENEW = 1;
		break;
		case 'C' :
		strncpy(tempstring,optarg,19);
		CONSPIRACYSIZE = atoi(tempstring);
		if(CONSPIRACYSIZE < 0) { fprintf(stdout,"Bad value entered.\n"); exit(1); }
		break;
		}//switch
			
	}//while
	//Move to end of Argument List
	argc -= optind; 
	argv += optind;
	
	//Create new seed
	if(SEED == 0)
		SEED = (int)(time(NULL));	
	
	srand(SEED);
	
  
  // make people
  people = (Person**)malloc(PEOPLE * sizeof(Person*));
  for (i=0; i<PEOPLE; i++) {
    people[i]=makePerson(i, CREATENEW);
  }

  groupsizes = (int*) malloc(ORGS* sizeof(int));
  // group 0 is everyone
  groupsizes[0] = PEOPLE;
  // group 1 is a conspiracy
  groupsizes[1] = CONSPIRACYSIZE;
  // other groups sizes are randomly chosen
  for (i=2; i<ORGS; i++) {
    groupsizes[i] = rand() % (MAXGROUPSIZE-MINGROUPSIZE+1) + MINGROUPSIZE;
  }

  groupfiles = (int*) malloc(ORGS* sizeof(int));
  // group 0 is everyone
  groupfiles[0] = NUMSYSTEMFILES;
  // group 1 is a conspiracy
  groupfiles[1] = NUMCONSPIRACYFILES;
  // other groups sizes are randomly chosen
  for (i=2; i<ORGS; i++) {
    groupfiles[i] = rand() % (MAXGROUPFILES-MINGROUPFILES+1) + MINGROUPFILES;
  }

  // make groups
  orgs = (Group**)malloc(ORGS * sizeof(Group*));
  for (i=0; i<ORGS; i++) {
    orgs[i]=makeGroup(i, groupsizes[i], groupfiles[i], people, CREATENEW);
  }
  
  // tell a person about the groups they are in
  informPeopleOfTheirGroups(people, orgs);

  printf("Seed used to create simulation: %d\n",SEED);
  outputSeed(SEED);
  // simulate the passage of time
  
  for (t=0; t<SIMDURATION; t++) {
    printf(">>> time is now %d\n", t);

    // run the people
    for (i=0; i<PEOPLE; i++) {
      simPerson(people[i], groupfiles);
    }

    // run the groups
    for (i=0; i<ORGS; i++) {
      simGroup(orgs[i]);
    }
  }
  
  
}

