#include "ast.h"
#include "common/list.h"
#include "common/map.h"
#include "token.h"
#include "util.h"
#include "llvm-c/Core.h"
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

int main() {

    SourceFile* f = sourcefile_malloc_read("test/input.ch");

    // sourcefile_print(f, true);

    List* tokens = sourcefile_tokenize(f);
    // token_print_list(tokens);

    ASTNode* root = ast_build(tokens, f);

    ast_node_print(root, true, 0);

    list_free(tokens);
    sourcefile_free(f);

    return 0;
}