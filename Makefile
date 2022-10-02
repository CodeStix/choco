all: src/main.cpp src/token.cpp src/ast.cpp
	rm -f a.out
	rm -rf build
	mkdir build
	clang++ -g -O0 -c `llvm-config-14 --cxxflags` -I/usr/lib/llvm-10/include src/main.cpp -o build/main.o
	clang++ -g -O0 -c `llvm-config-14 --cxxflags` -I/usr/lib/llvm-10/include src/token.cpp -o build/token.o
	clang++ -g -O0 -c `llvm-config-14 --cxxflags` -I/usr/lib/llvm-10/include src/ast.cpp -o build/ast.o
	clang++ -g -O0 -c `llvm-config-14 --cxxflags` -I/usr/lib/llvm-10/include src/typedValue.cpp -o build/typedValue.o
	clang++ -g -O0 -c `llvm-config-14 --cxxflags` -I/usr/lib/llvm-10/include src/util.cpp -o build/util.o
	clang++ build/*.o `llvm-config-14 --ldflags --libs` -lpthread -lncurses -o build/output

run: all
	./build/output