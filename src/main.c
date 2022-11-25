#include "list.h"
#include "token.h"
#include "llvm-c/Core.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main() {

    SourceFile* f = sourcefile_malloc_read("test/input.ch");

    sourcefile_print(f, true);

    List* tokens = sourcefile_tokenize(f);
    token_print_list(tokens);

    list_free(tokens);
    sourcefile_free(f);

    return 0;
}