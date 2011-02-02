CC = gcc
#LIBS = -lssl -lmysqlclient
LIBS = -lssl -L/usr/lib/mysql/ -lmysqlclient

DEFINES = -DDEFAULT_USER=\"snapshot\" -DDEFAULT_PASS=\"snapshot\" -DNOSPIN

all: snapshot query ncd simpleshowdata anomaly collaboration anomaly-curve

snapshot:
	$(CC)  snapshot.c hashFunctions.c mysql.c -o snapshot $(LIBS) $(DEFINES)

query:
	$(CC)  query.c hashFunctions.c mysql.c -o query $(LIBS) $(DEFINES)

ncd:
	$(CC)  ncd.c mysql.c -o ncd -lm -lzip $(LIBS) $(DEFINES)
	
simpleshowdata:
	$(CC)  simpleshowdata.c mysql.c -o simpleshowdata $(LIBS) $(DEFINES)

anomaly:
	$(CC)  anomaly.c mysql.c -o anomaly $(LIBS) $(DEFINES)

collaboration:
	$(CC)  collaboration.c mysql.c -o collaboration $(LIBS) $(DEFINES)

anomaly-curve:
	$(CC)  anomaly-curve.c mysql.c -o anomaly-curve $(LIBS) $(DEFINES)

	
clean:
	rm snapshot 
	rm query
	rm ncd 
	rm simpleshowdata
	rm anomaly
	rm collaboration
	rm anomaly-curve
