/*
 * This is bin2c program, which allows you to convert binary file to
 * C language array, for use as embedded resource, for instance you can
 * embed graphics or audio file directly into your program.
 * This is public domain software, use it on your own risk.
 * Contact Serge Fukanchik at fuxx@mail.ru  if you have any questions.
 *
 * Some modifications were made by Gwilym Kuiper (kuiper.gwilym@gmail.com)
 * I have decided not to change the licence.
 *
 * Extra modifications to output .c and .h files made by kociumba
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef USE_BZ2
#include <bzlib.h>
#endif

int main(int argc, char* argv[]) {
    char* buf;
    char* ident;
    char* output_base;
    char* c_filename;
    char* h_filename;
    unsigned int i, file_size, need_comma;

    FILE *f_input, *f_output_c, *f_output_h;

#ifdef USE_BZ2
    char* bz2_buf;
    unsigned int uncompressed_size, bz2_size;
#endif

    if (argc < 4) {
        fprintf(stderr, "Usage: %s binary_file output_base array_name\n", argv[0]);
        fprintf(
            stderr,
            "  output_base: base name for output files (will generate output_base.c and "
            "output_base.h)\n"
        );
        return -1;
    }

    f_input = fopen(argv[1], "rb");
    if (f_input == NULL) {
        fprintf(stderr, "%s: can't open %s for reading\n", argv[0], argv[1]);
        return -1;
    }

    // Get the file length
    fseek(f_input, 0, SEEK_END);
    file_size = ftell(f_input);
    fseek(f_input, 0, SEEK_SET);

    buf = (char*)malloc(file_size);
    assert(buf);

    fread(buf, file_size, 1, f_input);
    fclose(f_input);

#ifdef USE_BZ2
    // allocate for bz2.
    bz2_size = (file_size + file_size / 100 + 1) + 600;  // as per the documentation

    bz2_buf = (char*)malloc(bz2_size);
    assert(bz2_buf);

    // compress the data
    int status = BZ2_bzBuffToBuffCompress(bz2_buf, &bz2_size, buf, file_size, 9, 1, 0);

    if (status != BZ_OK) {
        fprintf(stderr, "Failed to compress data: error %i\n", status);
        return -1;
    }

    // and be very lazy
    free(buf);
    uncompressed_size = file_size;
    file_size = bz2_size;
    buf = bz2_buf;
#endif

    output_base = argv[2];
    c_filename = (char*)malloc(strlen(output_base) + 3);
    h_filename = (char*)malloc(strlen(output_base) + 3);
    assert(c_filename && h_filename);

    sprintf(c_filename, "%s.c", output_base);
    sprintf(h_filename, "%s.h", output_base);

    f_output_c = fopen(c_filename, "w");
    if (f_output_c == NULL) {
        fprintf(stderr, "%s: can't open %s for writing\n", argv[0], c_filename);
        return -1;
    }

    f_output_h = fopen(h_filename, "w");
    if (f_output_h == NULL) {
        fprintf(stderr, "%s: can't open %s for writing\n", argv[0], h_filename);
        fclose(f_output_c);
        return -1;
    }

    ident = argv[3];

    fprintf(f_output_c, "#include \"%s.h\"\n\n", output_base);

    need_comma = 0;

    fprintf(f_output_c, "const char %s[%i] = {", ident, file_size);
    for (i = 0; i < file_size; ++i) {
        if (need_comma)
            fprintf(f_output_c, ", ");
        else
            need_comma = 1;
        if ((i % 11) == 0) fprintf(f_output_c, "\n\t");
        fprintf(f_output_c, "0x%.2x", buf[i] & 0xff);
    }
    fprintf(f_output_c, "\n};\n\n");

    fprintf(f_output_c, "const int %s_length = %i;\n", ident, file_size);

#ifdef USE_BZ2
    fprintf(f_output_c, "const int %s_length_uncompressed = %i;\n", ident, uncompressed_size);
#endif

    // Write the .h file
    fprintf(f_output_h, "#ifndef %s_H\n", ident);
    fprintf(f_output_h, "#define %s_H\n\n", ident);

    fprintf(f_output_h, "#ifdef __cplusplus\n");
    fprintf(f_output_h, "extern \"C\" {\n");
    fprintf(f_output_h, "#endif\n\n");

    fprintf(f_output_h, "extern const char %s[%i];\n", ident, file_size);
    fprintf(f_output_h, "extern const int %s_length;\n\n", ident);

    fprintf(f_output_h, "#ifdef __cplusplus\n");
    fprintf(f_output_h, "}\n");
    fprintf(f_output_h, "#endif\n");

#ifdef USE_BZ2
    fprintf(f_output_h, "extern const int %s_length_uncompressed;\n", ident);
#endif

    fprintf(f_output_h, "\n#endif /* %s_H */\n", ident);

    fclose(f_output_c);
    fclose(f_output_h);

    free(buf);

    free(c_filename);
    free(h_filename);

    return 0;
}
