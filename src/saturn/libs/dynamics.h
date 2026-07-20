#ifndef DYNAMICS_H
#define DYNAMICS_H

#include <stddef.h>
#include <stdbool.h>

#define smart __attribute__((cleanup(__container_cleanup)))

typedef int(*CompareFunc)(const void*, const void*);

#define _GENERIC(T) typeof(typeof(T)*)
#define list(T) _GENERIC(T)
#define set(T) _GENERIC(T)
#define stack(T) _GENERIC(T)
#define queue(T) _GENERIC(T)
#define map(K, V) typedef struct { typeof(K) key; typeof(V) value; }

#define size(container) __container_size(container)
#define push(container) (*(typeof(container))__container_push((void**)&(container)))
#define pop(container) ((typeof(container))__container_pop(container))
#define peek(container, ...) ((typeof(container))__container_peek((container), (size_t){__VA_ARGS__}))
#define find(container, ptr) ((typeof(container))__container_find((container), (ptr)))
#define take(container, index) ((typeof(container))__container_remove((container), (index)))
#define indexof(ptr) __container_indexof((ptr), sizeof(*(ptr)))
#define base(ptr) __container_base((ptr), sizeof(*(ptr)))
#define clear(container) __container_clear(container)
#define destroy(container) __container_destroy(container)
#define add(container) push(container) // alias

size_t __container_size(void* container);
void* __container_push(void** container);
void* __container_pop(void* container);
void* __container_peek(void* container, size_t how_much);
void* __container_find(void* container, void* ptr);
void* __container_remove(void* container, size_t index);
void* __container_base(void* container, size_t item_size);
size_t __container_indexof(void* container, size_t item_size);
void __container_clear(void* container);
void __container_destroy(void* container);
void __container_cleanup(void* container);

#define list_init(T) ((list(T))__list_init(sizeof(T)))
#define set_init(T, compare) ((set(T))__set_init(sizeof(T), compare))
#define stack_init(T) ((stack(T))__stack_init(sizeof(T)))
#define queue_init(T) ((stack(T))__queue_init(sizeof(T)))

void* __list_init(size_t item_size);
void* __set_init(size_t item_size, CompareFunc compare);
void* __stack_init(size_t item_size);
void* __queue_init(size_t item_size);

#define foreach(item, container) \
    for (size_t __iter = 0, __broken = 0; __iter < __builtin_choose_expr( \
        (!__builtin_types_compatible_p(typeof(container), typeof(&(container)[0]))), \
        sizeof(typeof(container)) / (size_t)sizeof(*(container)), size(container) \
    ) && !__broken; __iter++) \
    for (typeof(*(container)) item##_[0], item = __builtin_choose_expr( \
        __builtin_types_compatible_p(typeof(item##_), typeof(&(container)[0])), \
        &(container)[__iter], (container)[__iter] \
    ), *__ptr = &(item); __ptr; __ptr = NULL) \
    for (__broken = 1; __broken; __broken = 0)

int compare_i8(const void*, const void*);
int compare_i16(const void*, const void*);
int compare_i32(const void*, const void*);
int compare_i64(const void*, const void*);
int compare_u8(const void*, const void*);
int compare_u16(const void*, const void*);
int compare_u32(const void*, const void*);
int compare_u64(const void*, const void*);
int compare_float(const void*, const void*);
int compare_double(const void*, const void*);
int compare_str(const void*, const void*);

#endif
