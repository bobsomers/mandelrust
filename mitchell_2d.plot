set title "Mitchell-Netravali Weight Distribution"
set xlabel "Sample Offset (x)"
set ylabel "Sample Offset (y)"
set zlabel "Weight"
set tics
set terminal png size 800,600
set output 'mitchell_2d.png'
splot "mitchell_2d.dat" notitle
