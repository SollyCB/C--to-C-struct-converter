#ifndef SOL_ARRAY_H_INCLUDE_GUARD_
#define SOL_ARRAY_H_INCLUDE_GUARD_

#include <stdlib.h>

// Backend
inline static void* fn_new_array(int cap, int width) {
    int *ret = malloc(cap * width + 16);
    ret[0] = width * cap;
    ret[1] = 0;
    ret[2] = width;
    ret += 4;
    return (void*)ret;
}
inline static void* fn_realloc_array(int *array) {
    if (array[0] > array[1] * array[2])
        return array + 4;
    array = realloc(array, array[0] * 2 + 16);
    assert(array);
    array[0] *= 2;
    array += 4;
    return array;
}

// Frontend
#define new_array(cap, type) fn_new_array(cap, sizeof(type))
#define free_array(array) free((int*)array - 4)

#define array_cap(array)   (((int*)array)[-4] / ((int*)array)[-2])
#define array_len(array)   ((int*)array)[-3]
#define array_inc(array)   ((int*)array)[-3]++
#define array_dec(array)   ((int*)array)[-3]--

#define array_add(array, elem) \
    array = fn_realloc_array((int*)array - 4); \
    array[array_len(array)] = elem; \
    array_inc(array)
#define array_pop(array) \
    if (array_len(array) != 0) \
        array_dec(array) \
    else \
        false

#endif // include guard
