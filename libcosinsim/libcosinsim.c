#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <math.h>

#define DEFAULT_CHUNK 16000
#define DEFAULT_SHOW 10
#define DEFAULT_TINYCHUNK 8000

#define optsearchcount 8
//Options for setopt
#define D_OFFSET 0
#define D_COMPRESSION_TYPE 1
#define D_CHUNK_SIZE 2
#define D_RANDOM_OFFSET 3
#define D_DOUBLE 4
#define D_TINY_CHUNK_SIZE 5
#define D_RANDOM_K 6
#define D_SHOW_WARNING 7
char *optsearch[optsearchcount] = { "OFFSET", "COMPRESSION_TYPE", "CHUNK_SIZE", "RANDOM_OFFSET", "DOUBLE", "TINY_CHUNK_SIZE", "RANDOM_K", "SHOW_WARNING" };

//int setoption(char *, void *);
int histogram(char *, int [], ssize_t, int, int *);
int histogramFiles(char *, char *, int, ssize_t, float *);

//Flags
int GLOBAL_SECONDSTAGE = 0;
int GLOBAL_DOUBLE = 0;
int GLOBAL_CVSOUT = 0;
float GLOBAL_BASE = 0;
int GLOBAL_RANDOM = 0;
int GLOBAL_RANDOMK = 0;
int GLOBAL_COMPARE = 0;
int GLOBAL_CHUNK = DEFAULT_CHUNK;
int TINY_CHUNK = DEFAULT_TINYCHUNK;
int SHOW_WARNING = 1;
ssize_t GLOBAL_OFFSET = 0;

int Abs(int x)
{
int t = x >> 31; 
return t ^ (x + t);
}

int setoption(char *string, long int option)
{
	char *sub_ptr;
	int count;
	
	
	for(count = 0; count < optsearchcount; count++)
	{
		sub_ptr = NULL;
		if( ( sub_ptr = strstr(string, optsearch[count])) != NULL )
		{
			switch(count)
			{

				case D_CHUNK_SIZE :
					if(option < 0) { return -1; }
					GLOBAL_CHUNK = option;
					break;
				case D_RANDOM_OFFSET :
					if(option < 0) { return -1; }
					GLOBAL_RANDOM = option;
					break;
				case D_DOUBLE :
					if(option < 0) { return -1; }
					GLOBAL_DOUBLE =  option;
					break;
				case D_TINY_CHUNK_SIZE :
					if(option < 0) { return -1; }
					TINY_CHUNK = option;
					break;		
				case D_RANDOM_K :
					if(option < 0) { return -1; }
					GLOBAL_RANDOMK = option;
					break;						
				case D_SHOW_WARNING :
					if(option < 0) { return -1; }
					SHOW_WARNING = option;
					break;	
				case D_OFFSET :
					if((ssize_t) option < 0) { return -1; }
					GLOBAL_OFFSET = (ssize_t) option;
					break;
			}
			return 0;
		}
	}
	return -1;
}

inline float Q_sqrt( float number )
{
        long i;
        float x2, y;
        const float threehalfs = 1.5F;
 
        x2 = number * 0.5F;
        y  = number;
        i  = * ( long * ) &y;                       
        i  = 0x5f3759df - ( i >> 1 );               // what the fudge
        y  = * ( float * ) &i;
        y  = y * ( threehalfs - ( x2 * y * y ) );   
		y  = y * ( threehalfs - ( x2 * y * y ) );   
        return number * y;
}



inline int histogram(char *filename, int histogram[], ssize_t offset, int chunk, int *total)
{
	FILE *histfile;
	int c;
	int inc = 0;
	for (c = 0; c < 256; c++)
	{
		histogram[c] = 0;
	}
	if( (histfile = fopen(filename, "rb")) == NULL) return -1;
	if(offset > 0) if( (fseek(histfile, offset, SEEK_SET)) != 0) { if(SHOW_WARNING) fprintf(stderr,"Error seeking file %s.\n",filename); return -1;}
	if( chunk == 0)
	{
		do {
		  c = getc (histfile);
		  histogram[c]++;
		  inc++;
		} while (c != EOF);
	}
	else
	{
		do {
		  c = getc (histfile);
		  histogram[c]++;
		  inc++;
		} while (inc <= chunk && c != EOF);	
	}
	*total = inc;
	fclose(histfile);
	return 0;
}


int cosinsimFiles(char *file, char *filetwo, int CHUNK, ssize_t offset, float *distance)
{
	int histogramone[256];
	int histogramtwo[256];
	int c;
	int numerator = 0;
	long int denominator = 0;
	long int denominatortwo = 0;
	int totalbytes, totalbytestwo;
	*distance = 0;
	histogram(file, histogramone, offset, CHUNK, &totalbytes);
	histogram(filetwo, histogramtwo, offset, CHUNK, &totalbytestwo);
	if(totalbytes > totalbytestwo) totalbytes = totalbytestwo;
	for(c=0; c < 256; c++)
		{
			numerator += (histogramtwo[c]*histogramone[c]);
		}
	for(c=0; c < 256; c++)
		{
			denominator += (histogramone[c]*histogramone[c]);
		}	
	for(c=0; c < 256; c++)
		{
			denominatortwo += (histogramtwo[c]*histogramtwo[c]);
		}	
	
	
//	*distance = (float) numerator / ( Q_sqrt(denominator) * Q_sqrt(denominatortwo));	
	*distance = (float) numerator / ( sqrt(denominator) * sqrt(denominatortwo));	

	return 0;
}


void distanceFunction(char *file1, char *file2, float *distance, float *ddistance)
{
		*distance = 0; *ddistance =0;
		cosinsimFiles(file1, file2, GLOBAL_CHUNK, GLOBAL_OFFSET, distance);
		//Inverse Distance
		*distance = 1 - *distance;
		//if(*distance < 0) *distance = 0;
}



