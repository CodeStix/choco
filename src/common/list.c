#include "common/list.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

struct List {
    void** items;
    unsigned int capacity;
    unsigned int length;
};

List* list_malloc(int initial_cap) {
    List* l = (List*)malloc(sizeof(List));
    l->capacity = initial_cap;
    l->length = 0;
    l->items = malloc(sizeof(void*) * initial_cap);
    return l;
}

void list_add(List* list, void* item) {
    if (list->length >= list->capacity) {
        unsigned int new_cap = list->capacity * 2 + 1;
        list->items = realloc(list->items, sizeof(void*) * new_cap);
        list->capacity = new_cap;
    }

    list->items[list->length] = item;
    list->length++;
}

void list_remove(List* list, unsigned int index) {
    assert(index < list->length);
    for (int i = index; i < list->length - 1; i++) {
        list->items[i] = list->items[i + 1];
    }
    list->length--;
}

void* list_get(List* list, unsigned int index) {
    assert(index < list->length);
    return list->items[index];
}

void list_free(List* list) {
    free(list->items);
    free(list);
}

unsigned int list_length(List* list) {
    return list->length;
}

unsigned int list_capacity(List* list) {
    return list->capacity;
}