all: main.cpp
	rm -f a.out
	g++ -lstdc++ main.cpp

run: all
	./a.out