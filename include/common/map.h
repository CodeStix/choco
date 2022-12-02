#ifndef MAP_H
#define MAP_H

#include <stdbool.h>

typedef struct Map Map;
typedef struct MapNode MapNode;
typedef struct MapIterator MapIterator;

typedef unsigned long (*HashCodeCalculatorFunc)(void* data);
typedef int (*ComparatorFunc)(void* a, void* b);

#define MAP_DEFAULT_TABLE_SIZE 16

Map* map_malloc(unsigned int table_size, ComparatorFunc comp_func, HashCodeCalculatorFunc hash_func);
Map* map_malloc_string_comparator(unsigned int table_size);
void* map_put(Map* map, void* key, void* value);
void* map_get(Map* map, void* key, void* default_value);
void* map_delete(Map* map, void* key);
void map_clear(Map* map);
void map_free(Map* map);
unsigned int map_length(Map* map);
void map_print(Map* map, char* key_value_format);

MapIterator* map_iter_begin(Map* map);
bool map_iter_next(MapIterator* iterator, void** out_key, void** out_value);
void map_iter_end(MapIterator* iter);

#endif   // MAP_H
