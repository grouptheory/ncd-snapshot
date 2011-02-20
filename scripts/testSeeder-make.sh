#!/bin/bash
TOTAL=1000
NAME="duration"
DIR="/home/ralcalde/Code/ncd-snapshot/test/"
TEMPC=0
USER="snapshot"
PASS="snapshot"
CONSPIRACY=4

rm -rf duration* person*
#Start first part of creation
./company -c -d 0 -C $CONSPIRACY
mkdir ${NAME}0
cp -R person* ${NAME}0

#intialize
#../snapshot -u $USER -p $PASS -I
#../query -u $USER -p $PASS -I
#cp SEEDS SEEDS-RUN-START
#make directories
SEED=$(cat SEED)
echo Our seed $SEED


for i in `seq 1 $TOTAL`;
do
	echo ${NAME}${i}
	./company -d 1 -s $SEED -C $CONSPIRACY
	mkdir ${NAME}${i}
	cp -R person* ${NAME}${i}
#	cd ${NAME}${i}
#	TEMPC=0
#	for f in person-*; 
#	do 
#		echo "Processing $f"; 
#		if [ "$TEMPC" -eq "0" ] 
#			then 
#			../../snapshot -u $USER -p $PASS -N -c 0 -i "Rich" -m ${NAME}${i} ${DIR}${NAME}${i}/${f}/
#			TEMPC=1
#			else
#			../../snapshot  -u $USER -p $PASS -c $i -i "Rich" -m ${NAME}${i} ${DIR}${NAME}${i}/${f}/
#		fi
#	done
#	cd ..
#	../query  -u $USER -p $PASS -K $i -q
#	../ncd -u $USER -p $PASS -c 32000 -q
	cp SEEDS SEEDS-RUN-NUMBER-${i}
done


