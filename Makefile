all: prog
	./mandel > mandel.ppm
	convert mandel.ppm mandel.png
	open mandel.png

prog: mandel.rs
	rustc mandel.rs

clean:
	rm mandel.ppm mandel.png mandel
