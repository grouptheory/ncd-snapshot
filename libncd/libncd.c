#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h> //for path size
#include <fcntl.h>
#include <zlib.h>
#include <math.h>



#define sqlbufsize 255
#define DEFAULT_CHUNK 16000
#define DEFAULT_SHOW 10
#define DEFAULT_TINYCHUNK 8000


#define optsearchcount 7
//Options for setopt
#define D_OFFSET 0
#define D_COMPRESSION_TYPE 1
#define D_CHUNK_SIZE 2
#define D_RANDOM_OFFSET 3
#define D_DOUBLE 4
#define D_TINY_CHUNK_SIZE 5
#define D_RANDOM_K 6
char *optsearch[optsearchcount] = { "OFFSET", "COMPRESSION_TYPE", "CHUNK_SIZE", "RANDOM_OFFSET", "DOUBLE", "TINY_CHUNK_SIZE", "RANDOM_K" };

int compressSize(char *, ssize_t *, ssize_t *, ssize_t *, int , int, ssize_t );
int combineSize(char *, char *, ssize_t *, ssize_t *, ssize_t *, int , int, ssize_t );
void NCDtwofiles(char *, char *, int , int, ssize_t, float *, float * );
void NCDtwofilesRand(char *, char *, int, int, int, float *, float *);
int checkfile(const char *);
ssize_t getFileSize(char *);


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
int GLOBAL_COMPRESSION = Z_DEFAULT_COMPRESSION;
ssize_t GLOBAL_OFFSET = 0;



int setoption(char *string, const long int option)
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
				case D_OFFSET :
					if((ssize_t)option < 0) { return -1; }
					GLOBAL_OFFSET = (ssize_t) option;
					break;
				case D_COMPRESSION_TYPE :
					if(option < 0) { return -1; }
					GLOBAL_COMPRESSION = option;
					break;
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
					GLOBAL_DOUBLE = option;
					break;
				case D_TINY_CHUNK_SIZE :
					if(option < 0) { return -1; }
					TINY_CHUNK = option;
					break;		
				case D_RANDOM_K :
					if(option < 0) { return -1; }
					GLOBAL_RANDOMK = option;
					break;						
			}
			return 0;
		}
	}
	return -1;
}

int compressSize(char *file, ssize_t *osize, ssize_t *csize, ssize_t *dsize, int level, int CHUNK, ssize_t offset)
{
	int ret;
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
	int ret;
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



void distanceFunction(char *file1, char *file2, float *ncd, float *dncd)
{
		*ncd = 0; *dncd =0;
		if(GLOBAL_RANDOM == 0)
			NCDtwofiles(file1, file2, GLOBAL_COMPRESSION, GLOBAL_CHUNK, GLOBAL_OFFSET, ncd, dncd);
		else 
			NCDtwofilesRand(file1, file2, Z_DEFAULT_COMPRESSION, GLOBAL_CHUNK, GLOBAL_RANDOMK, ncd, dncd);	    
}



