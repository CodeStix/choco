
#ifndef UTIL_H
#define UTIL_H

#include <stdbool.h>

bool is_operator(char c);
char* file_read_all_contents(char* source_path, unsigned long* out_size);

#endif   // UTIL_H