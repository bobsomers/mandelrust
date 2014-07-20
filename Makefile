all: compile
	./mandel > mandel.ppm
	convert mandel.ppm mandel.png
	rm mandel.ppm
	open mandel.png

plot: compile
	./mandel
	gnuplot halton23.plot; open halton23.png
	gnuplot mitchell_1d.plot; open mitchell_1d.png
	gnuplot mitchell_2d.plot; open mitchell_2d.png

compile: mandel.cc
	clang++ -std=c++11 -Wall -Wextra -o mandel mandel.cc

#all: compile
#	./mandel > mandel.ppm
#	convert mandel.ppm mandel.png
#	rm mandel.ppm
#	open mandel.png
#
#compile: mandel.rs
#	rustc mandel.rs

clean:
	rm mandel.ppm mandel.png mandel \
	   halton23.dat mitchell_1d.dat mitchell_2d.dat \
	   halton23.png mitchell_1d.png mitchell_2d.png
