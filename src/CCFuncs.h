#ifndef CCFUNCS_H
#define CCFUNCS_H

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <stdarg.h>
#include <string.h>

// DYNAMIC ARRAY //

// Code taken from: https://github.com/tsoding/nob.h
#define DA_INIT_CAP 128

#define da_append(da, item)                                                          \
    do {                                                                             \
        if((da)->count >= (da)->capacity) {                                          \
            (da)->capacity = (da)->capacity == 0 ? DA_INIT_CAP : (da)->capacity*2;   \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "No enough ram");                          \
        }                                                                            \
                                                                                     \
        (da)->items[(da)->count++] = (item);                                         \
    } while(0)

#define da_free(da) do { free((da)->items); } while(0)

#define da_append_many(da, new_items, new_items_count)                                  \
    do {                                                                                    \
        if ((da)->count + (new_items_count) > (da)->capacity) {                               \
            if ((da)->capacity == 0) {                                                      \
                (da)->capacity = DA_INIT_CAP;                                           \
            }                                                                               \
            while ((da)->count + (new_items_count) > (da)->capacity) {                        \
                (da)->capacity *= 2;                                                        \
            }                                                                               \
            (da)->items = realloc((da)->items, (da)->capacity*sizeof(*(da)->items)); \
            assert((da)->items != NULL && "No enough ram");                          \
        }                                                                                   \
        memcpy((da)->items + (da)->count, (new_items), (new_items_count)*sizeof(*(da)->items)); \
        (da)->count += (new_items_count);                                                     \
    } while (0)
// end of taken code

// printf like function that prints the name and line of the file where it was called
#define log_error(msg, ...) _log_error(msg, __FILE__, __LINE__, __VA_ARGS__);

// STRING BUILDER //

typedef struct {
    char *items;
    size_t count;
    size_t capacity;
} StringBuilder;

// dumps a null terminated string
char *sb_dump_str(StringBuilder *sb);

// ARENA //
typedef struct Region Region;

struct Region {
    void *data;
    size_t count;
    size_t capacity;
};

// Arena is basically a dynamic array of Regions
typedef struct {
    Region *items;
    size_t count;
    size_t capacity;

    size_t regionSize;
    size_t regionIndex;
} Arena;

Arena *arena_create(size_t regionSize);
void *arena_alloc(Arena *arena, size_t size);
void arena_clear(Arena *arena); // clears the arena (NOTE: no region or allocation is freed)
void arena_free(Arena *arena);

#endif // CCFUNCS_H

#ifdef CCFUNCS_IMPLEMENTATION

void _log_error(const char *msg, char *file, int line, ...) {
    printf("[ERROR]: ");

    va_list args;
    va_start(args, line);
    vprintf(msg, args);
    va_end(args);

    printf(" (at %s:%d)\n", file, line);
}

char *sb_dump_str(StringBuilder *sb) {
    char *str = malloc(sb->count + 1);
    strncpy(str, sb->items, sb->count);
    str[sb->count] = '\0';
    return str;
}

Arena *arena_create(size_t regionSize) {
    Arena *arena = calloc(1, sizeof(Arena));
    assert(arena != NULL && "Not enough memory");
    arena->regionSize = regionSize;
    return arena;
}

void *arena_alloc(Arena *arena, size_t size) {
    assert(size <= arena->regionSize && "Size cannot be bigger than region size");

    if(arena->count == 0) {
        da_append(arena, ((Region) {
            .data = malloc(arena->regionSize),
            .capacity = arena->regionSize,
        }));
    }

    Region *region = &arena->items[arena->regionIndex];

    if(region->count + size > region->capacity) {
        if(arena->regionIndex >= arena->count - 1) {
            da_append(arena, ((Region) {
                .data = malloc(arena->regionSize),
                .capacity = arena->regionSize,
            }));
        }

        region = &arena->items[++arena->regionIndex];
    }

    void *mem = region->data + region->count;
    region->count += size;
    return mem;
}

void arena_clear(Arena *arena) {
    arena->regionIndex = 0;

    // reset every region setting its count to 0
    for(size_t i = 0; i < arena->count; i++) {
        arena->items[i].count = 0;
    }
}

void arena_free(Arena *arena) {
    for(size_t i = 0; i < arena->count; i++) {
        free(arena->items[i].data);
    }

    da_free(arena);
    free(arena);
}

#endif // CCFUNCS_IMPLEMENTATION
