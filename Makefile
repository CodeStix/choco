
all: src/main.c
	rm -f a.out
	mkdir -p build build/ast
	
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -Iinclude/ -I/usr/lib/llvm-14/include src/main.c -o build/main.o
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -Iinclude/ -I/usr/lib/llvm-14/include src/util.c -o build/util.o
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -Iinclude/ -I/usr/lib/llvm-14/include src/list.c -o build/list.o
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -Iinclude/ -I/usr/lib/llvm-14/include src/token.c -o build/token.o
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -Iinclude/ -I/usr/lib/llvm-14/include src/ast.c -o build/ast.o
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -Iinclude/ -I/usr/lib/llvm-14/include src/ast/function.c -o build/ast/function.o
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -Iinclude/ -I/usr/lib/llvm-14/include src/ast/file.c -o build/ast/file.o

	clang -g -O0 -fno-limit-debug-info build/*.o build/ast/*.o `llvm-config-14 --ldflags --libs` -lpthread -lncurses -o build/output

clean:
	rm -rf build
	
