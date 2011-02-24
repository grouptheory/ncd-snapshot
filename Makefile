CC = gcc
#LIBS = -lssl -lmysqlclient
LIBS = -L/usr/lib/mysql/ -lmysqlclient

DEFINES = -DDEFAULT_USER=\"snapshot\" -DDEFAULT_PASS=\"snapshot\" -DNOSPIN

all: snapshot query simpleshowdata anomaly collaboration anomaly-curve company collaboration-csv distance

snapshot:
	$(CC)  snapshot.c hashFunctions.c mysql.c -o snapshot -lssl $(LIBS) $(DEFINES)

query:
	$(CC)  query.c hashFunctions.c mysql.c -o query $(LIBS) $(DEFINES)

distance:
	$(CC)  distance.c mysql.c -o distance -rdynamic -ldl $(LIBS) $(DEFINES)
	
simpleshowdata:
	$(CC)  simpleshowdata.c mysql.c -o simpleshowdata $(LIBS) $(DEFINES)

anomaly:
	$(CC)  anomaly.c mysql.c -o anomaly $(LIBS) $(DEFINES)

collaboration:
	$(CC)  collaboration.c mysql.c -o collaboration $(LIBS) $(DEFINES)

collaboration-csv:
	$(CC)  collaboration-csv.c mysql.c -o collaboration-graphing/collaboration-csv -pthread $(LIBS) $(DEFINES)

anomaly-curve:
	$(CC)  anomaly-curve.c mysql.c -o anomaly-curve $(LIBS) $(DEFINES)

company: 
	$(CC)  company.c -o company

	
clean:
	rm snapshot 
	rm query
	rm simpleshowdata
	rm anomaly
	rm collaboration
	rm anomaly-curve
	rm company
	rm ./collaboration-graphing/collaboration-csv
	rm distance
