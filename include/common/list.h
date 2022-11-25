#ifndef LIST_H
#define LIST_H

typedef struct List List;

List* list_malloc(int initial_cap);
void list_add(List* list, void* item);
void list_remove(List* list, unsigned int index);
void* list_get(List* list, unsigned int index);
void list_free(List* list);
unsigned int list_length(List* list);
unsigned int list_capacity(List* list);

#endif   // LIST_H