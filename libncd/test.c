#include<stdio.h>
#include<stdlib.h>
#include<dlfcn.h>

typedef void (*distanceFunction)(char *, char *, float *, float *);
typedef void (*setoptFunction)(char *, void *);

int main()
{
void *lib;
distanceFunction distance;
setoptFunction setopt;
const char * err;
float ncd, dncd;
ssize_t offset = 10;

lib = dlopen("/usr/local/lib/libncd.so.1.0", RTLD_NOW);
if(!lib)
	{
		printf("Failed to open: %s\n",dlerror());
		exit(1);
	}
dlerror();

distance = (distanceFunction) dlsym(lib,"distanceFunction");
err=dlerror();
if(err) 
	{
		printf("Failed to locate: %s\n",dlerror());
		exit(1);
	}
	
setopt = (setoptFunction) dlsym(lib,"setoption");
err=dlerror();
if(err) 
	{
		printf("Failed to locate: %s\n",dlerror());
		exit(1);
	}
	
setopt("OFFSET", (void *)offset);

distance("../company.c", "../ncd.c", &ncd, &dncd);
printf("NCD values %f %f\n", ncd, dncd);

dlclose(lib);
return 0;

}
