#ifndef LSYSTEMS_MEMORY_UTILS_H
#define LSYSTEMS_MEMORY_UTILS_H

#include <cstdlib>

void *safe_malloc(size_t size);

void *safe_calloc(size_t count, size_t size);

void *safe_realloc(void *ptr, size_t size);

void safe_free(void** ptr);

#endif //LSYSTEMS_MEMORY_UTILS_H
