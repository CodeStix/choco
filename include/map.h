#ifndef MAP_H
#define MAP_H

typedef struct Map Map;
typedef struct MapNode MapNode;

typedef unsigned long (*HashCodeCalculatorFunc)(void* data);
typedef int (*ComparatorFunc)(void* a, void* b);

Map* map_malloc(unsigned int table_size, ComparatorFunc comp_func, HashCodeCalculatorFunc hash_func);
Map* map_malloc_string_comparator(unsigned int table_size);
void* map_put(Map* map, void* key, void* value);
void* map_get(Map* map, void* key, void* default_value);
void* map_delete(Map* map, void* key);
void map_clear(Map* map);
void map_free(Map* map);

#endif   // MAP_H
