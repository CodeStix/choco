#include "common/map.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct MapNode {
    struct MapNode* next;
    void* key;
    void* value;
};

struct Map {
    HashCodeCalculatorFunc key_hasher;
    ComparatorFunc key_comparator;
    unsigned int table_size;
    unsigned int length;
    struct MapNode** table;
};

Map* map_malloc(unsigned int table_size, ComparatorFunc comp_func, HashCodeCalculatorFunc hash_func) {
    Map* m = malloc(sizeof(Map));
    m->table = malloc(table_size * sizeof(struct MapNode*));
    m->table_size = table_size;
    m->length = 0;
    m->key_hasher = hash_func;
    m->key_comparator = comp_func;

    for (unsigned int i = 0; i < table_size; i++) {
        m->table[i] = NULL;
    }

    return m;
}

int string_comparator(void* a, void* b) {
    return strcmp((char*)a, (char*)b);
}

// Source: http://www.cse.yorku.ca/~oz/hash.html
unsigned long simple_hasher(void* data) {
    char* str = (char*)data;
    unsigned long hash = 5381;
    int c;
    while ((c = *str++))
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    return hash;
}

Map* map_malloc_string_comparator(unsigned int table_size) {
    return map_malloc(table_size, string_comparator, simple_hasher);
}

MapNode* mapnode_malloc(void* key, void* value) {
    MapNode* n = malloc(sizeof(MapNode));
    n->key = key;
    n->value = value;
    n->next = NULL;
    return n;
}

unsigned long map_get_table_index(Map* map, void* key) {
    unsigned long hash = map->key_hasher(key);
    return hash % (unsigned long)map->table_size;
}

void* map_put(Map* map, void* key, void* value) {
    unsigned long table_index = map_get_table_index(map, key);

    MapNode** current_node_ptr = &map->table[table_index];

    while (true) {
        MapNode* current_node = *current_node_ptr;
        if (current_node == NULL) {
            *current_node_ptr = mapnode_malloc(key, value);
            map->length++;
            return NULL;
        }

        if (map->key_comparator(current_node->key, key) == 0) {
            void* prev_value = current_node->value;
            current_node->value = value;
            return prev_value;
        }

        current_node_ptr = &current_node->next;
    }
}

void* map_get(Map* map, void* key, void* default_value) {
    unsigned long table_index = map_get_table_index(map, key);

    MapNode* current_node = map->table[table_index];

    while (true) {
        if (current_node == NULL) {
            return default_value;
        }

        if (map->key_comparator(current_node->key, key) == 0) {
            return current_node->value;
        }

        current_node = current_node->next;
    }
}

void mapnode_free(MapNode* mn) {
    free(mn);
}

void* map_delete(Map* map, void* key) {
    unsigned long table_index = map_get_table_index(map, key);

    MapNode** current_node_ptr = &map->table[table_index];

    while (true) {
        MapNode* current_node = *current_node_ptr;
        if (current_node == NULL) {
            return NULL;
        }

        if (map->key_comparator(current_node->key, key) == 0) {
            *current_node_ptr = current_node->next;
            void* value = current_node->value;
            mapnode_free(current_node);
            map->length--;
            return value;
        }

        current_node_ptr = &current_node->next;
    }
}

void map_clear(Map* map) {
    for (unsigned int i = 0; i < map->table_size; i++) {
        MapNode* current_node = map->table[i];

        while (current_node != NULL) {
            MapNode* next_node = current_node->next;
            mapnode_free(current_node);
            current_node = next_node;
        }

        map->table[i] = NULL;
    }
    map->length = 0;
}

unsigned int map_length(Map* map) {
    return map->length;
}

void map_free(Map* map) {
    free(map->table);
    free(map);
}

struct MapIterator {
    Map* map;
    MapNode* current_node;
    int table_index;
};

MapIterator* map_iter_begin(Map* map) {
    MapIterator* i = malloc(sizeof(MapIterator));
    i->map = map;
    i->current_node = NULL;
    i->table_index = 0;
    return i;
}

bool map_iter_next(MapIterator* iterator, void** out_key, void** out_value) {

    if (iterator->current_node == NULL) {
        iterator->current_node = iterator->map->table[0];
    } else {
        if (iterator->current_node->next == NULL) {
            while (true) {
                iterator->table_index++;

                if (iterator->table_index >= iterator->map->table_size) {
                    return false;
                }

                iterator->current_node = iterator->map->table[iterator->table_index];
                if (iterator->current_node != NULL) {
                    break;
                }
            }
        } else {
            iterator->current_node = iterator->current_node->next;
        }
    }

    *out_key = iterator->current_node->key;
    *out_value = iterator->current_node->value;
    return true;
}

void map_iter_end(MapIterator* iter) {
    free(iter);
}

void map_print(Map* map, char* key_value_format) {
    MapIterator* iterator = map_iter_begin(map);

    void* value;
    void* key;
    while (map_iter_next(iterator, &key, &value)) {
        printf(key_value_format, key, value);
    }

    map_iter_end(iterator);
}