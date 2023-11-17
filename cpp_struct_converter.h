/*
                                        (license at bottom of file - MIT)
    -----------------------------------------------------------------------------------------------------------
                                                    ** API **
    -----------------------------------------------------------------------------------------------------------
    1. Define SOL_CPP_STRUCT_CONVERTER to include the C source code (STB style)
    2. Call to convert C++ style structs to C style:

           char* convert_structs(int len, char *data, int *new_len);

    @Note Seems pretty robust, but I have only tested it on regular code and the few edge cases I can think of.
    I am certain it will break to something, but it works fine on everything I have written.

*/

#ifndef SOL_CPP_STRUCT_CONVERTER_H_INCLUDE_GUARD_
#define SOL_CPP_STRUCT_CONVERTER_H_INCLUDE_GUARD_

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <string.h>
#include <stddef.h>
#include <stdbool.h>

char* convert_structs(int len, char *data, int *new_len);

#ifdef SOL_CPP_STRUCT_CONVERTER_IMPLEMENTATION

#define Max_s32 INT32_MAX

#define ZERO 48
#define NINE 57
#define UPPER_A 65
#define UPPER_Z 90
#define LOWER_A 97
#define LOWER_Z 122
#define UNDERSCORE 95

inline static int skip_whitespace(int len, char *data) {
    int i;
    for(i = 0; i < len; ++i) {
        switch(data[i]) {
        case ' ':
        case '\n':
            continue;
        default:
            return i;
        }
    }
    return Max_s32;
}
inline static int skip_to_whitespace(int len, char *data) {
    int i;
    for(i = 0; i < len; ++i) {
        switch(data[i]) {
        case ' ':
        case '\n':
            return i;
        default:
            break;
        }
    }
    return Max_s32;
    return i;
}
inline static bool is_valid_char(char c) { // valid characters before 'struct' keyword
    switch(c) {
    case ' ':
    case '\n':
    case ';':
        return true;
    default:
        return false;
    }
}
inline static bool is_struct(char *string) {
    return memcmp(string, "struct ", 7) == 0 && is_valid_char(string[-1]);
}
inline static int word_len(char *string) {
    int i;
    for(i = 0; true; ++i) {
        if (string[i] >= UPPER_A && string[i] <= UPPER_Z) {}
        else if (string[i] >= LOWER_A && string[i] <= LOWER_Z) {}
        else if (string[i] >= ZERO && string[i] <= NINE) {}
        else if (string[i] == UNDERSCORE) {}
        else {return i;}
    }
}
inline static int struct_len(char *string) {
    int stack = 0;
    int i;
    for(i = 0; true; ++i) {
        if (string[i] == '{')
            stack++;
        if (string[i] == '}') {
            stack--;
            if (stack == 0)
                return i + 1;
        }
    }
}

// This is so small that it can be inlined in the same header which is nice
#ifndef SOL_ARRAY_H_INCLUDE_GUARD_
#define SOL_ARRAY_H_INCLUDE_GUARD_

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

#endif // array include guard

char* convert_structs(int len, char *data, int *new_len) {
    int *marks = new_array(128, int); // beginning of each struct
    int *name_lens = new_array(128, int); // lengths of the type names
    int *struct_lens = new_array(128, int); // char count from curly brace to curly brace
    int extra_size = 0; // size added by typedefs and doubled typenames
    int pos = 0;
    int tmp;
    while(len - pos > 6) {
        pos += skip_whitespace(len - pos, data + pos);
        if (is_struct(data + pos)) {
            array_add(marks, pos);

            // Skip to the type name
            pos += skip_to_whitespace(len - pos, data + pos);
            pos += skip_whitespace(len - pos, data + pos);

            // Find the length of the name
            tmp = word_len(data + pos);
            extra_size += tmp;
            array_add(name_lens, tmp);

            // Skip to the opening curly brace
            pos += skip_to_whitespace(len - pos, data + pos);
            pos += skip_whitespace(len - pos, data + pos);
            assert(data[pos] == '{' && "Not a curly brace opening a struct");

            // Find char count between braces including braces
            tmp = struct_len(data + pos);
            array_add(struct_lens, tmp);
            extra_size += tmp;
            pos += tmp;
        }
        pos += skip_to_whitespace(len - pos, data + pos);
    }

    // Allocate the return buffer, with enough size to include the typedefs and double type names
    char *ret = malloc(len + extra_size);

    int count = array_len(marks);
    int last_mark = 0;

    int current_mark = 0;
    int current_name_len = 0;
    int current_struct_len = 0;

    char name_buffer[128];

    pos = 0;
    for(int i = 0; i < count; ++i) {
        current_mark = marks[i];
        current_name_len = name_lens[i];
        current_struct_len = struct_lens[i];
        memcpy(ret + pos, data + last_mark, current_mark - last_mark);

        // Cursor on 's' of struct
        tmp = last_mark;
        last_mark += current_mark - tmp;
        pos += current_mark - tmp;

        // Step beyond 'struct'
        last_mark += 6;
        last_mark += skip_whitespace(len - last_mark, data + last_mark);

        // Copy the type name into intermediate buffer
        memcpy(name_buffer, data + last_mark, name_lens[i]);
        last_mark += current_name_len + 1;
        assert(data[last_mark] == '{' && "Should be pointing at struct opening");

        // Copy in "typedef " to return buffer
        memcpy(ret + pos, "typedef ", 8);
        pos += 8;

        // Copy in "struct " to return buffer
        memcpy(ret + pos, "struct ", 7);
        pos += 7;

        // Copy in the type name after "typedef struct "
        memcpy(ret + pos, name_buffer, current_name_len);
        pos += current_name_len;

        // Add a space after the type name
        ret[pos] = ' ';
        pos++;

        // Copy in the struct data (everything inside curly braces, including the braces: "{..}")
        memcpy(ret + pos, data + last_mark, current_struct_len);
        last_mark += current_struct_len;
        pos += current_struct_len;
        pos++;
        assert(data[last_mark-1] == '}' && "Should be pointing at struct end");

        // Add a space after the closing curly brace
        ret[pos] = ' ';
        pos++;

        // Copy in the type name again after the closing curly brace
        memcpy(ret + pos, name_buffer, current_name_len);
        pos += current_name_len;
    }

    // Copy in all the data after the last struct
    memcpy(ret + pos, data + last_mark, len - last_mark);

    // Fill in the returned new file len, including the added typedefs and doubled typenames
    *new_len = len + extra_size;

    // Print final contents
    for(int i = 0; i < *new_len; ++i)
        printf("%c", ret[i]);

    return ret;
}
#endif // implementation guard
#endif // include guard
/*

    Copyright 2023 Solomon Carden Brown (solomoncardenbrown@gmail.com)

    Permission is hereby granted, free of charge, to any person obtaining a
    copy of this software and associated documentation files (the “Software”),
    to deal in the Software without restriction, including without limitation
    the rights to use, copy, modify, merge, publish, distribute, sublicense,
    and/or sell copies of the Software, and to permit persons to whom the
    Software is furnished to do so, subject to the following conditions:

    The above copyright notice and this permission notice shall be included in
    all copies or substantial portions of the Software.

    THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
    FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.

*/

