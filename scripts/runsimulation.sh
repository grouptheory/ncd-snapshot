#!/bin/bash
if [ -z "$1" ]; then
        echo usage: $0 SEEDX SEEDY
        exit
#
TOTAL=1000
NAME="duration"
DIR="/home/ralcalde/Code/ncd-snapshot/test/"
TEMPC=0
USER="snapshot"
PASS="snapshot"
CONSPIRACY=4

rm -rf duration* person* SEED* 

#Start first part of creation
./company -c -d 0 -C $CONSPIRACY --seedx $1 --seedy $2
mkdir ${NAME}0
cp -R person* ${NAME}0

#intialize
../snapshot -u $USER -p $PASS -I
../query -u $USER -p $PASS -I


for i in `seq 1 $TOTAL`;
do
	echo ${NAME}${i}
        SEEDX=$(cat SEED-X)
        SEEDY=$(cat SEED-Y)
        echo Our seed X is $SEEDX
        echo Our seed Y is $SEEDY
	./company -d 1 --seedx $SEEDX --seedy $SEEDY -C $CONSPIRACY
	mkdir ${NAME}${i}
	cp -R person* ${NAME}${i}
	cd ${NAME}${i}
	TEMPC=0
	for f in person-*; 
	do 
		echo "Processing $f"; 
		if [ "$TEMPC" -eq "0" ] 
			then 
			../../snapshot -u $USER -p $PASS -N -c 0 -i "Rich" -m ${NAME}${i} ${DIR}${NAME}${i}/${f}/
			TEMPC=1
			else
			../../snapshot  -u $USER -p $PASS -c $i -i "Rich" -m ${NAME}${i} ${DIR}${NAME}${i}/${f}/
		fi
	done
	cd ..
	time ../query  -u $USER -p $PASS -K $i -q
	time ../distance -N -u $USER -p $PASS -c 32000 -q
done


