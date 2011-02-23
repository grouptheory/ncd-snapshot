CC = gcc
#LIBS = -lm -lzip
LIBS = -lm -L/usr/bin -lcygz

#DEFINES = -DDEFAULT_USER=\"snapshot\" -DDEFAULT_PASS=\"snapshot\" -DNOSPIN

all: libncd test

test:
	$(CC) -rdynamic  test.c -o test
#	$(LIBS) $(DEFINES)


libncd:
	$(CC) -Wall $(LIBS) -c -fPIC libncd.c
	$(CC) -shared $(LIBS) -Wl,-soname,libncd.so.1 -o libncd.so.1.0  libncd.o

#$(LIBS) $(DEFINES)

clean:
	rm *.o
	rm *.exe
	rm *.so.*