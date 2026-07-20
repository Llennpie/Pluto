#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "timsort.h"

typedef int(*CompareFunc)(const void*, const void*);

#define MAGIC 0xB00B

typedef enum {
    Container_List,
    Container_Set,
    Container_Queue,
} ContainerType;

typedef struct {
    uint32_t magic;
    ContainerType type;
    bool dirty;
    bool is_pointer;
    size_t item_size;
    size_t size, capacity;
    union {
        CompareFunc compare;
        struct { size_t head, tail; };
    };
} Header;

size_t __container_size(void* container) {
    Header* header = (Header*)container - 1;
    if (header->magic != MAGIC) return 0;
    return header->size;
}

void __container_destroy(void* container) {
    if (!container) return;

    Header* header = (Header*)container - 1;
    if (header->magic != MAGIC) return;
    free(header);
}

void __container_cleanup(void* container) {
    if (!container) return;
    __container_destroy(*(void**)container);
}

static void* list_push(void** container) {
    Header* header = (Header*)*container - 1;
    if (header->size == header->capacity) {
        header->capacity *= 2;
        header = realloc(header, sizeof(Header) + header->capacity * header->item_size);
        *container = header + 1;
    }
    header->dirty = true;
    return (uint8_t*)*container + header->item_size * header->size++;
}

static void* list_pop(void* container) {
    Header* header = (Header*)container - 1;
    return (uint8_t*)container + header->item_size * --header->size;
}

static void* list_peek(void* container, size_t how_much) {
    Header* header = (Header*)container - 1;
    return (uint8_t*)container + header->item_size * (header->size - how_much - 1);
}

static void* list_find(void* container, void* what) {
    Header* header = (Header*)container - 1;
    for (size_t i = 0; i < header->size; i++) {
        if (memcmp(container + header->item_size * i, what, header->item_size) == 0)
            return container + header->item_size * i;
    }
    return NULL;
}

static void* list_remove(void* container, size_t index) {
    Header* header = (Header*)container - 1;
    if (header->size == 0 || index >= header->size) return NULL;
    header->size--;
    if (index != header->size) {
        char buf[header->item_size];
        memcpy(buf, container + index * header->item_size, header->item_size);
        memmove(
            container + index * header->item_size,
            container + (index + 1) * header->item_size,
            header->item_size * (header->size - index)
        );
        memcpy(container + header->size * header->item_size, buf, header->item_size);
    }
    return container + header->size * header->item_size;
}

static void list_clear(void* container) {
    Header* header = (Header*)container - 1;
    header->size = 0;
}

static void* set_pop(void* container) {
    return NULL;
}

static void* set_find(void* container, void* what) {
    Header* header = (Header*)container - 1;
    if (header->dirty) {
        header->dirty = false;
        timsort(container, header->size, header->item_size, header->compare);
    }
    return bsearch(what, container, header->size, header->item_size, header->compare);
}

static void* queue_push(void** container) {
    Header* header = (Header*)*container - 1;
    if (header->size == header->capacity) {
        header->capacity *= 2;
        Header* new = malloc(sizeof(Header) + header->item_size * header->capacity);
        uint8_t* new_items = (uint8_t*)(new + 1);
        uint8_t* old_items = *container;
        memcpy(new, header, sizeof(Header));
        memcpy(new_items, old_items + new->tail * new->item_size, new->item_size * (new->size - new->tail));
        memcpy(new_items + (new->size - new->tail) * new->item_size, old_items, new->item_size * new->tail);
        free(header);
        header = new;
        header->tail = 0;
        header->head = header->size;
        *container = new_items;
    }
    header->size++;
    void* ptr = (uint8_t*)*container + header->head * header->item_size;
    header->head = (header->head + 1) % header->capacity;
    return ptr;
}

static void* queue_pop(void* container) {
    Header* header = (Header*)container - 1;
    if (header->size == 0) return NULL;
    header->size--;
    void* ptr = (uint8_t*)container + header->tail * header->item_size;
    header->tail = (header->tail + 1) % header->capacity;
    return ptr;
}

static void* queue_peek(void* container, size_t how_much) {
    Header* header = (Header*)container - 1;
    return (uint8_t*)container + (header->tail + how_much) % header->capacity * header->item_size;
}

static void* queue_find(void* container, void* what) {
    Header* header = (Header*)container - 1;
    size_t index = header->tail;
    for (size_t i = 0; i < header->size; i++) {
        if (memcmp(container + header->item_size * index, what, header->item_size) == 0)
            return container + header->item_size * index;
        index = (index + 1) % header->capacity;
    }
    return NULL;
}

static void* queue_remove(void* container, size_t index) {
    return NULL; // unimplemented
}

static void queue_clear(void* container) {
    Header* header = (Header*)container - 1;
    header->size = header->tail = header->head = 0;
}

void* __container_push(void** container) {
    Header* header = (Header*)*container - 1;
    if (header->magic != MAGIC) return NULL;
    return (void*(*[])(void**)){
        list_push, list_push, queue_push,
    }[header->type](container);
}

void* __container_pop(void* container) {
    Header* header = (Header*)container - 1;
    if (header->magic != MAGIC || header->size == 0) return NULL;
    return (void*(*[])(void*)){
        list_pop, set_pop, queue_pop
    }[header->type](container);
}

void* __container_peek(void* container, size_t how_much) {
    Header* header = (Header*)container - 1;
    if (header->magic != MAGIC || header->size == 0) return NULL;
    return (void*(*[])(void*, size_t)){
        list_peek, list_peek, queue_peek
    }[header->type](container, how_much);
}

void* __container_find(void* container, void* what) {
    Header* header = (Header*)container - 1;
    if (header->magic != MAGIC) return NULL;
    return (void*(*[])(void*, void*)){
        list_find, set_find, queue_find
    }[header->type](container, what);
}

void* __container_remove(void* container, size_t index) {
    Header* header = (Header*)container - 1;
    if (header->magic != MAGIC) return NULL;
    return (void*(*[])(void*, size_t)){
        list_remove, list_remove, queue_remove
    }[header->type](container, index);
}

void* __container_base(void* container, size_t item_size) {
    while (((Header*)container)[-1].magic != MAGIC)
        container = (uint8_t*)container - item_size;
    return container;
}

size_t __container_indexof(void* container, size_t item_size) {
    void* base = __container_base(container, item_size);
    return ((uintptr_t)container - (uintptr_t)base) / item_size;
}

void __container_clear(void* container) {
    Header* header = (Header*)container - 1;
    if (header->magic != MAGIC) return;
    (void(*[])(void*)){
        list_clear, list_clear, queue_clear,
    }[header->type](container);
}

static void* init_container(size_t item_size, ContainerType type) {
    Header* header = calloc(sizeof(Header) + item_size * 4, 1);
    header->capacity = 4;
    header->magic = MAGIC;
    header->type = type;
    header->item_size = item_size;
    return header + 1;
}

void* __list_init(size_t item_size) {
    return init_container(item_size, Container_List);
}

void* __set_init(size_t item_size, CompareFunc compare) {
    Header* header = init_container(item_size, Container_Set);
    header[-1].compare = compare;
    return header;
}

void* __stack_init(size_t item_size) {
    return init_container(item_size, Container_List);
}

void* __queue_init(size_t item_size) {
    return init_container(item_size, Container_Queue);
}

#define compare(name, type) \
    int compare_##name(const void* a, const void* b) { \
        return (*(typeof(type)*)a > *(typeof(type)*)b) - (*(typeof(type)*)a < *(typeof(type)*)b); \
    }

compare(i8, int8_t)
compare(i16, int16_t)
compare(i32, int32_t)
compare(i64, int64_t)
compare(u8, uint8_t)
compare(u16, uint16_t)
compare(u32, uint32_t)
compare(u64, uint64_t)
compare(float, float)
compare(double, double)

int compare_str(const void* a, const void* b) {
    return strcmp(*(char**)a, *(char**)b);
}