all: src/main.cpp src/token.cpp src/ast.cpp
	rm -f a.out
	g++ -lstdc++ src/main.cpp src/token.cpp src/ast.cpp

run: all
	./a.out