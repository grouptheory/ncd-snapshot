#!/bin/bash
TOTAL=20

for i in `seq 1 $TOTAL`;
do
	echo Showing Anomaly Data for Query $i out of $TOTAL
	../anomaly -Q $i
	../collaboration
done

