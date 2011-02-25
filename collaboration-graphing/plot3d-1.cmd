set term png size 1024, 768
set output "graphic3d-iteration-1.png"
set dgrid3d 20,20,20  
set pm3d
splot "3d.dat" using 1:2:3 with lines

