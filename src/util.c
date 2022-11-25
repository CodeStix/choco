#include "util.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

bool is_operator(char c) {
    switch (c) {
    case '=':
    case '+':
    case '-':
    case '*':
    case '/':
    case '>':
    case '<':
    case '!':
    case '|':
    case '&':
    case '^':
    case '~':
    case '%':
    case '?':
    case '#':
        return true;
    default:
        return false;
    }
}

char* file_read_all_contents(char* source_path, unsigned long* out_size) {
    FILE* fd = fopen(source_path, "r");
    assert(fd != NULL);

    fseek(fd, 0, SEEK_END);
    unsigned long size = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    char* file_contents = malloc(size * sizeof(char));
    assert(fread(file_contents, sizeof(char), size, fd) == size);
    assert(fclose(fd) == 0);

    *out_size = size;
    return file_contents;
}