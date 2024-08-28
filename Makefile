all : lab1

lab1: lab1.cpp
	g++ lab1.cpp -Wall -lX11 -lGL -lGLU -lm -o lab1

clean:
	rm lab1 -f