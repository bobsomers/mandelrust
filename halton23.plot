set title "Halton 2,3 Sequence (2x2 Pixel Area)"
set xlabel "Sample Offset (x)"
set ylabel "Sample Offset (y)"
set tics
set terminal png size 800,600
set output 'halton23.png'
plot "halton23.dat" notitle
