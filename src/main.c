#include "ast.h"
#include "list.h"
#include "map.h"
#include "token.h"
#include "util.h"
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

    ASTNode* root = ast_build(tokens, f);

    ast_node_print(root);

    list_free(tokens);
    sourcefile_free(f);

    Map* m = map_malloc_string_comparator(16);
    map_put(m, "nice", "this is epic");
    map_put(m, "main", "calculate");
    map_put(m, "getint", "calculate");
    map_put(m, "exported", "something ele");

    printf("%s\n", map_get(m, "nice", "?"));
    printf("%s\n", map_get(m, "main", "?"));
    printf("%s\n", map_get(m, "getint", "?"));
    printf("%s\n", map_get(m, "exported", "?"));

    map_free(m);

    return 0;
}