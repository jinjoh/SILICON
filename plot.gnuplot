#!/usr/bin/gnuplot -persist
set pm3d
set palette rgb 33,13,10
set view map
splot '/tmp/plane.dat' with points palette pt 0.2 ps 0.1
