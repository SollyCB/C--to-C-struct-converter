/*
                                        (license at bottom of file - MIT)
    -----------------------------------------------------------------------------------------------------------
                                                    ** API **
    -----------------------------------------------------------------------------------------------------------
    1. Define SOL_CPP_STRUCT_CONVERTER to include the C source code (STB style)
    2. Call to convert C++ style structs and enums to C style:

           char* convert_structs(int len, char *data, int *new_len);

    @Note Seems pretty robust, but I have only tested it on regular code and the few edge cases I can think of.
          I am certain it will break to something, but it works fine on everything I have written.
    @Note If a struct or enum is wrapped in a scope, they will be unaffected; for instance, if they are
          wrapped in a namespace.

    I apologise for the lack of doc comments, its not normally my style to leave stuff undocumented, but
    this thing is so trivial, I did not think that it really mattered.

*/

#ifndef SOL_CPP_STRUCT_CONVERTER_H_INCLUDE_GUARD_
#define SOL_CPP_STRUCT_CONVERTER_H_INCLUDE_GUARD_

#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

char* convert_struct_and_enums(int len, char *data, int *new_len, bool structs, bool enums);

#ifdef SOL_CPP_STRUCT_CONVERTER_IMPLEMENTATION

#define Max_s32 INT32_MAX

inline static int skip_whitespace(int len, char *data) {
    int i;
    for(i = 0; i < len; ++i) {
        if (data[i] != ' ' && data[i] != '\n')
            return i;
    }
    return Max_s32;
}

inline static int skip_name_chars(int len, char *data) {
    int i;
    for(i = 0; i < len; ++i) {
        if (data[i] >= '0' && data[i] <= '9') {continue;}
        if (data[i] >= 'A' && data[i] <= 'Z') {continue;}
        if (data[i] >= 'a' && data[i] <= 'z') {continue;}
        if (data[i] == '_') {continue;}
        return i;
    }
    return Max_s32;
}

inline static int skip_to_name_chars(int len, char *data) {
    int i;
    for(i = 0; i < len; ++i) {
        if (data[i] >= 'A' && data[i] <= 'Z') {break;}
        if (data[i] >= 'a' && data[i] <= 'z') {break;}
        if (data[i] == '_') {break;}
        continue;
    }
    return Max_s32;
}

inline static int skip_to_equivalent_scope(int len, char *data) {
    int stack = 0;
    int i;
    for(i = 0; i < len; ++i) {
        if (data[i] == '{')
            stack++;
        if (data[i] == '}')
            stack--;
        if (stack == 0)
            return i+1;
    }
    return Max_s32;
}

inline static int skip_comment(int len, char *data) {
    int i = 1;
    if (data[i] == '*') {
        for(i = 2; i < len; ++i)
            if (data[i] == '*' && data[i+1] == '/')
                return i+2;
    } else if (data[i] == '/') {
        for(i = 2; i < len; ++i)
            if (data[i] == '\n' && data[i-1] != '\\')
                return i+1;
    } else {
        return i;
    }
}

inline static int skip_macro(int len, char *data) {
    int i;
    for(i = 1; i < len; ++i)
        if (data[i] == '\n' && data[i-1] != '\\')
            return i;
    return Max_s32;
}

inline static int find_char(int len, char *data, char c) {
    int i;
    for(i = 0; i < len; ++i)
        if (data[i] == c)
            return i;
    return Max_s32;
}

inline static int get_name_len(int len, char *data) {
    int i;
    for(i = 0; i < len; ++i) {
        if (data[i] >= '0' && data[i] <= '9') {continue;}
        if (data[i] >= 'A' && data[i] <= 'Z') {continue;}
        if (data[i] >= 'a' && data[i] <= 'z') {continue;}
        if (data[i] == '_') {continue;}
        return i;
    }
    return Max_s32;
}

typedef enum {
    STRUCT,
    ENUM,
    NAN,
} Keyword;

inline static Keyword match_keyword(int len, char *data) {
    if (memcmp(data, "struct ", 7) == 0)
        return STRUCT;
    if (memcmp(data, "enum ", 5) == 0)
        return ENUM;
    return NAN;
}

typedef struct {
    int len;
    const char *str;
} String;

typedef struct {
    char *buf;
    int len;
    int cap;
} String_Buffer;

inline static String_Buffer new_string_buffer(int size) {
    String_Buffer ret;
    ret.len = 0;
    ret.cap = size;
    ret.buf = malloc(size);
    return ret;
}

inline static void free_string_buffer(String_Buffer *buf) {
    free(buf->buf);
}

inline static String string_buffer_get_string(String_Buffer *buf, String *str) {
    String ret = {};
    ret.len = str->len;

    if (ret.len + 1 + buf->len >= buf->cap) {
        buf->cap *= 2;
        buf->buf = realloc(buf->buf, buf->cap);
    }

    ret.str = (const char*)(buf->buf + buf->len);

    buf->len += ret.len + 1;
    assert(buf->len <= buf->cap && "String Buffer Overflow");

    memcpy((char*)ret.str, str->str, ret.len);

    char *tmp = (char*)ret.str;
    tmp[ret.len] = '\0';

    return ret;
}

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

typedef struct {
    int pos;
    int len;
    String name;
    bool forward_declaration;
} Token;

inline static String parse_name(int len, char *data, int name_len) {
    int pos = name_len;

    assert(data[pos] == ' ' || data[pos] == '\n' &&
          "This should be a whitespace separating struct keyword and struct name");

    pos += skip_whitespace(len - pos, data + pos);

    String name = {
        .len=get_name_len(len - pos, data + pos),
        .str=data + pos,
    };
    return name;
}

inline static int distance_to_closing_brace(int len, char *data) {
    int stack = 0;

    int i = find_char(len, data, '{');
    int j = find_char(len, data, ';');

    for(i; i < len; ++i) {
        if (data[i] == '{')
            stack++;
        if (data[i] == '}')
            stack--;
        if (stack == 0)
            return i+1;
    }
    return Max_s32;
}

char* convert_structs_and_enums(int len, char *data, int *new_sz, bool structs, bool enums) {
    String_Buffer name_buf = new_string_buffer(1024);

    Token *token_array = new_array(128, Token);
    Token  token;

    int pos = skip_whitespace(len, data);
    int extra_size = 0;
    while(pos < len && pos >= 0) {
        switch(data[pos]) {
        case '/':
            pos += skip_comment(len - pos, data + pos);
            break;
        case '#':
            pos += skip_macro(len - pos, data + pos);
            break;
        default:
            skip_to_name_chars(len - pos, data + pos);

            switch(match_keyword(len - pos, data + pos)) {
            case STRUCT:
                if (structs) {
                    token.pos  = pos;
                    token.name = parse_name(len - pos, data + pos, 6);
                    token.name = string_buffer_get_string(&name_buf, &token.name);
                    extra_size += token.name.len + 10;

                    if (find_char(len - pos, data + pos, ';') < find_char(len - pos, data + pos, '{')) {
                        token.forward_declaration = true;
                        token.len = 0;
                        array_add(token_array, token);
                        pos += find_char(len - pos, data + pos, ';');
                        break;
                    } else {
                        token.forward_declaration = false;
                    }

                    token.len = distance_to_closing_brace(len - pos, data + pos);
                    array_add(token_array, token);
                    pos += find_char(len - pos, data + pos, '{');
                    pos += skip_to_equivalent_scope(len - pos, data + pos);
                } else {
                    pos++; // Hackish; who cares, its a trivial cli
                }
                break;
            case ENUM:
                if (enums) {
                    token.pos   = pos;
                    token.name  = parse_name(len - pos, data + pos, 4);
                    token.name  = string_buffer_get_string(&name_buf, &token.name);
                    extra_size += token.name.len + 10;

                    if (find_char(len - pos, data + pos, ';') < find_char(len - pos, data + pos, '{')) {
                        token.forward_declaration = true;
                        token.len = 0;

                        array_add(token_array, token);

                        pos += find_char(len - pos, data + pos, ';');
                        break;
                    } else {
                        token.forward_declaration = false;
                    }

                    token.len = distance_to_closing_brace(len - pos, data + pos);
                    array_add(token_array, token);
                    pos += find_char(len - pos, data + pos, '{');
                    pos += skip_to_equivalent_scope(len - pos, data + pos);
                } else {
                    pos++; // Hackish; who cares, its a trivial cli
                }
                break;
            default:
                pos++;
                break;
            }
        }
        pos += skip_whitespace(len - pos, data + pos);
    }
    char *ret = malloc(len + extra_size);
    int p1 = 0;
    int p2 = 0;
    int count = array_len(token_array);
    for(int i = 0; i < count; ++i) {
        token = token_array[i];
        memcpy(ret + p1, data + p2, token.pos - p2);
        p1 += token.pos - p2;
        p2 += token.pos - p2;

        memcpy(ret + p1, "typedef ", 8);
        p1 += 8;

        memcpy(ret + p1, data + p2, token.len);
        p1 += token.len;
        p2 += token.len;

        if (token.forward_declaration)
            continue;

        ret[p1] = ' ';
        p1++;
        memcpy(ret + p1, token.name.str, token.name.len);
        p1 += token.name.len;
    }

    memcpy(ret + p1, data + p2, len - p2);

    while(ret[len + extra_size - 1] != '\n')
        extra_size--;

    *new_sz = extra_size + len;
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

