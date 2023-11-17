#define SOL_CPP_STRUCT_CONVERTER_IMPLEMENTATION
#include "cpp_struct_converter.h"

int main(int argc, const char *argv[]) {
    FILE *file = fopen(argv[1], "rb");
    fseek(file, 0, SEEK_END);
    int len = ftell(file);
    fseek(file, 0, SEEK_SET);
    char *data = malloc(len);
    fread(data, 1, len, file);

    int new_len;
    char *out = convert_structs(len, data, &new_len);
    file = fopen(argv[2], "wb");
    fwrite(out, 1, new_len, file);

    return 0;
}
