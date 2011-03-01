CC = gcc
#LIBS = 
LIBS = -lm -L/usr/bin -lcygz

#DEFINES = -DDEFAULT_USER=\"snapshot\" -DDEFAULT_PASS=\"snapshot\" -DNOSPIN

all: libcosinsim test

test:
	$(CC) -rdynamic -ldl  test.c -o test
#	$(LIBS) $(DEFINES)


libcosinsim:
	$(CC)  -c -fPIC -mfpmath=sse -march=core2 -lm libcosinsim.c 
	$(CC)  -shared -lm -Wl,-soname,libcosinsim.so.1 -o libcosinsim.so.1.0  libcosinsim.o

#$(LIBS) $(DEFINES)

install:
	cp libcosinsim.so.* /usr/local/lib
	ldconfig

clean:
	rm *.o
	rm *.so.1.0
	rm test
	
