
#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

void free_null(void* ptr_or_null);
bool is_operator(char c);
char* file_read_all_contents(char* source_path, unsigned long* out_size);

#endif   // UTIL_H