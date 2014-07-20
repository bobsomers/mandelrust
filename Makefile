all: prog
	./mandel > mandel.ppm
	convert mandel.ppm mandel.png
	rm mandel.ppm
	open mandel.png

prog: mandel.rs
	rustc mandel.rs

clean:
	rm mandel.ppm mandel.png mandel
