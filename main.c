#define SOL_CPP_STRUCT_CONVERTER_IMPLEMENTATION
#include "cpp_struct_converter.h"

#include <string.h>
#include <stdio.h>

#define PRINT_HELP_MESSAGE \
        printf("    Convert C++ syntax to C:\n"); \
        printf("        usage:       cpptoc <file_to_read> <options>\n\n"); \
        printf("    Options:\n"); \
        printf("        -help:       Display this message\n"); \
        printf("        -write-file: Give a file write output to, e.g. \"-write-file out.c\"\n"); \
        printf("        -structs:    Convert C++ style structs to C style, e.g:\n"); \
        printf("                         C++: struct Thing {...};\n"); \
        printf("                         C:   typedef struct Thing {...} Thing;\n");

int main(int argc, const char *argv[]) {

    if (argc == 1) {
        PRINT_HELP_MESSAGE;
        return 0;
    }
    if (memcmp(argv[1], "-help", 5) == 0) {
        PRINT_HELP_MESSAGE;
        return 0;
    }

    FILE *file = fopen(argv[1], "rb");

    if (!file) {
        printf("The first argument must be the file to read (try -help)\n");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(len);
    fread(data, 1, len, file);

    bool do_convert_structs = false;
    bool do_convert_enums   = false;
    const char *out_file = NULL;

    bool no_matches = true;

    int new_len;
    char *out;
    for(int i = 2; i < argc; ++i)
        if (memcmp(argv[i], "-structs", 8) == 0) {
            do_convert_structs = true;
            no_matches = false;
        } else if (memcmp(argv[i], "-enums", 6) == 0) {
            do_convert_enums = true;
            no_matches = false;
        } else if (memcmp(argv[i], "-write-file", 11) == 0) {
            i++;
            out_file = argv[i];
            no_matches = false;
        }

    if (no_matches) {
        PRINT_HELP_MESSAGE;
    }

    if (do_convert_structs || do_convert_enums)
        out = convert_structs_and_enums(len, data, &new_len, do_convert_structs, do_convert_enums);

    if (out_file) {
        file = fopen(out_file, "wb");
        fwrite(out, 1, new_len, file);
    }

    return 0;
}
