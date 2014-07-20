set title "Mitchell-Netravali Weight Distribution"
set xlabel "Sample Offset (x)"
set ylabel "Weight"
set tics
set terminal png size 800,600
set output 'mitchell_1d.png'
plot "mitchell_1d.dat" notitle
