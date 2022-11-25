
all: src/main.c
	rm -f a.out
	mkdir -p build
	
	clang -g -O0 -fno-limit-debug-info -c `llvm-config-14 --cflags` -I/usr/lib/llvm-14/include src/main.c -o build/main.o

	clang -g -O0 -fno-limit-debug-info build/*.o `llvm-config-14 --ldflags --libs` -lpthread -lncurses -o build/output

clean:
	rm -rf build
	
