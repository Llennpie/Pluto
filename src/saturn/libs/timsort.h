#ifndef TIMSORT_H
#define TIMSORT_H

#include <stddef.h>

void timsort(void* base, size_t count, size_t size, int(*compare)(const void*, const void*));

#endif