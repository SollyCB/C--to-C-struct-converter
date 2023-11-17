#define SOL_CPP_STRUCT_CONVERTER_IMPLEMENTATION
#include "cpp_struct_converter.h"

#include <string.h>

int main(int argc, const char *argv[]) {

    FILE *file = fopen(argv[1], "rb");

    if (!file) {
        printf("The first argument must be the file to read...\n");
        return 0;
    }

    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(len);
    fread(data, 1, len, file);

    bool do_convert_structs = false;
    const char *out_file = NULL;

    int new_len;
    char *out;
    for(int i = 2; i < argc; ++i)
        if (memcmp(argv[i], "-structs", 8) == 0) {
            do_convert_structs = true;
        } else if (memcmp(argv[i], "-write-file", 11) == 0) {
            i++;
            out_file = argv[i];
        }

    if (do_convert_structs)
        out = convert_structs(len, data, &new_len);

    if (out_file) {
        file = fopen(out_file, "wb");
        fwrite(out, 1, new_len, file);
    }

    return 0;
}
