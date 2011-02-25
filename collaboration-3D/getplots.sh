#!/bin/bash
if [ -z "$1" ]; then
	echo "Be Gentle - Test code only"
        echo usage: $0 iterations
        exit
fi

iterations=$1
rm gnuplot.cmd

for (( c=1; c <= $iterations; c = c + 1 ))
do
	./collaboration-3d -Q $c

	echo "set term jpeg size 1024, 768" >> gnuplot.cmd
	printf "set output \"graphic3d-iteration-%05d.jpg\"\n" $c >> gnuplot.cmd
	echo "set dgrid3d 20,20,20" >> gnuplot.cmd
	echo "set pm3d" >> gnuplot.cmd
	printf "splot \"plot3d-%05d.dat\" using 1:2:3 with lines\n" $c >> gnuplot.cmd
done

gnuplot < gnuplot.cmd

mencoder "mf://*.jpg" -mf fps=10 -o simulation.avi -ovc lavc -lavcopts vcodec=msmpeg4v2:vbitrate=800 

