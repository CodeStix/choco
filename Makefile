all: src/main.cpp src/token.cpp src/ast.cpp
	rm -f a.out
	rm -rf build
	mkdir build
	clang++ -O3 -c `llvm-config --cxxflags` -I/usr/lib/llvm-10/include src/main.cpp -o build/main.o
	clang++ -O3 -c `llvm-config --cxxflags` -I/usr/lib/llvm-10/include src/token.cpp -o build/token.o
	clang++ -O3 -c `llvm-config --cxxflags` -I/usr/lib/llvm-10/include src/ast.cpp -o build/ast.o
	clang++ build/*.o `llvm-config --ldflags --libs` -lpthread -lncurses -o build/output

run: all
	./build/output