all: main

JJP: JJP.cpp
	g++ -std=c++11 -pthread -Wall -O2 -c JJP.cpp
main: JJP main.cpp JJP.o
	g++ -std=c++11 -pthread -Wall -O2 -o main main.cpp JJP.o
clean:
	rm -f JJP.o main
