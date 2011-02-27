CC = gcc
#LIBS = 
#LIBS = -lm -L/usr/bin -lcygz

#DEFINES = -DDEFAULT_USER=\"snapshot\" -DDEFAULT_PASS=\"snapshot\" -DNOSPIN

all: libhisto test

test:
	$(CC) -rdynamic -ldl  test.c -o test
#	$(LIBS) $(DEFINES)


libhisto:
	$(CC) -c -fPIC libhisto.c 
	$(CC) -shared -Wl,-soname,libhistogram.so.1 -o libhistogram.so.1.0  libhisto.o

#$(LIBS) $(DEFINES)

#install:
#	cp libncd.so.* /usr/local/lib
#	ldconfig

clean:
	rm *.o
	rm *.so.1.0
	rm test
	
