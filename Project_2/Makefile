all: main

JJP: JJP.cpp
	g++ -Wall -O2 -c JJP.cpp
main: JJP main.cpp JJP.o
	g++ -Wall -O2 -o main main.cpp -JJP.o
clean:
	rm -f JJP.o main