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

    sourcefile_print(f, true);

    List* tokens = sourcefile_tokenize(f);
    token_print_list(tokens);

    ASTNode* root = ast_build(tokens, f);

    // ast_node_print(root);

    Token* to_mark = list_get(tokens, 1);
    Token* end_to_mark = list_get(tokens, 5);
    sourcefile_highlight(f, token_start_index(to_mark), token_start_index(end_to_mark) + token_length(end_to_mark));

    // unsigned int line_len;
    // char* line = sourcefile_get_line(f, 2, &line_len);
    // printf("Line: '%.*s'\n", line_len, line);

    Map* m = map_malloc_string_comparator(16);
    map_put(m, "nice", "this is epic");
    map_put(m, "main", "calculate");
    map_put(m, "getint", "calculate");
    map_put(m, "exported", "something ele");
    map_put(m, "adfs", "nsdlkjf");

    map_print(m, "%s -> %s\n");

    map_free(m);
    list_free(tokens);
    sourcefile_free(f);

    return 0;
}