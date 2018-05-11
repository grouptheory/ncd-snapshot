CC = gcc
LIBS = -lm -lz
#LIBS = -lm -L/usr/bin -lcygz

#DEFINES = -DDEFAULT_USER=\"snapshot\" -DDEFAULT_PASS=\"snapshot\" -DNOSPIN

all: libncd test

test:
	$(CC) -L/usr/lib -rdynamic -ldl  test.c -o test
#	$(LIBS) $(DEFINES)


libncd:
	$(CC) -Wall $(LIBS) -c -fPIC libncd.c
	$(CC) -shared $(LIBS) -Wl,-soname,libncd.so.1 -o libncd.so.1.0  libncd.o

#$(LIBS) $(DEFINES)

install:
	cp libncd.so.* /usr/local/lib
	ldconfig

clean:
	rm *.o
	rm *.so.1.0
	rm test

