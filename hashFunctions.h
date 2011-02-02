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


char hashtype[5];

/*
Overview: Create hash of any file.

Input: A string containing the file name.
Output: A string containing the hash.

*/
int hashfile(char */*Input file name*/, char */*hash output*/);


/*
Overview: Create hash of any string.

Input: A string to be hashed.
Output: A string containing the hash.

*/
void hashstring(char */*Input String*/, char */*Output Hash*/);

