#include "memory_utils.h"
#include <stdexcept>
#include <cerrno>
#include <cstring>

#define SAFE_WRAPPER(func, ...) \
    errno = 0; \
    void *__ptr = func(__VA_ARGS__); \
    if (__ptr == nullptr || errno != 0) { \
        throw std::runtime_error("Failed to allocate memory: " + std::string(strerror(errno))); \
    } \
    return __ptr;

#define POSITIVE_CHECK(var) \
    if (var <= 0) { \
        throw std::runtime_error("Cannot allocate memory of size <= 0. Got: " + std::to_string(var) + " for " #var); \
    }


void *safe_malloc(size_t size) {
    POSITIVE_CHECK(size);
    SAFE_WRAPPER(malloc, size);
}

void *safe_calloc(size_t count, size_t size) {
    POSITIVE_CHECK(count);
    POSITIVE_CHECK(size);
    SAFE_WRAPPER(calloc, count, size);
}

void *safe_realloc(void *ptr, size_t size) {
    POSITIVE_CHECK(size);
    SAFE_WRAPPER(realloc, ptr, size);
}

void safe_free(void** ptr) {
    if (*ptr == nullptr) {
        throw std::runtime_error("Cannot free nullptr");
    }
    free(*ptr);
    *ptr = nullptr;
}
