
all: mandel mandelmovie

movie: ffmpeg ffplay mandelmovie
	./ffmpeg -i mandel%d.bmp mandel.mpg
	./ffplay mandel.mpg

mandelmovie: mandelmovie.c 
	gcc -Wall -lm mandelmovie.c -o mandelmovie

mandel: mandel.o bitmap.o
	gcc -Wall mandel.o bitmap.o -o mandel -lpthread

mandel.o: mandel.c
	gcc -Wall -g -c mandel.c -o mandel.o

bitmap.o: bitmap.c
	gcc -Wall -g -c bitmap.c -o bitmap.o

clean:
	rm -f mandel.o bitmap.o mandel mandelmovie mandel*.bmp mandel.mpg
