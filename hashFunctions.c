#include<stdio.h>
#include<fcntl.h>
#include<unistd.h>
#include<stdlib.h>
#include<string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <openssl/evp.h>
//#include <openssl/crypto.h>
#include<math.h>

char hashtype[]="sha1";

/*
Overview: Create hash of any file.

Input: A string containing the file name.
Output: A string containing the hash.

*/
int hashfile(char *thefile, char *hash)
{
	EVP_MD_CTX mdctx;
	const EVP_MD *md;
	size_t filecount;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	int md_len, i;
	
	

	md = EVP_get_digestbyname(hashtype);
	if(!md) exit(1);
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);


	int file;
	if( (file = open(thefile, O_RDONLY)) == -1 ) {return -1;}
	char tempbuffer[1000];
	while( (filecount=read(file,tempbuffer,sizeof(tempbuffer))) == sizeof(tempbuffer) )
	{
		EVP_DigestUpdate(&mdctx, tempbuffer, sizeof(tempbuffer));
		//printf("In %d\n ",sizeof(tempbuffer));
	}
		//printf("Out %d\n ",filecount);

	EVP_DigestUpdate(&mdctx, tempbuffer, filecount);

	EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);
	sprintf(hash,"\"");
	for(i = 0; i < md_len; i++) sprintf(hash,"%s%02x", hash,md_value[i]);
	strcat(hash,"\"");
	close(file);
	return 0;
}

/*
Overview: Create hash of any string.

Input: A string to be hashed.
Output: A string containing the hash.

*/
void hashstring(char *thestring, char *hash)
{
	EVP_MD_CTX mdctx;
	const EVP_MD *md;
	int count;
	unsigned char md_value[EVP_MAX_MD_SIZE];
	int md_len, i;
	
	OpenSSL_add_all_digests();

	md = EVP_get_digestbyname(hashtype);
	if(!md) exit(1);
	EVP_MD_CTX_init(&mdctx);
	EVP_DigestInit_ex(&mdctx, md, NULL);

	EVP_DigestUpdate(&mdctx, thestring, strlen(thestring));

	EVP_DigestFinal_ex(&mdctx, md_value, &md_len);
	EVP_MD_CTX_cleanup(&mdctx);
	sprintf(hash,"");
	for(i = 0; i < md_len; i++) sprintf(hash,"%s%02x", hash,md_value[i]);
}




