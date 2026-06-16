#include <stdio.h>
#include <stdlib.h>
#include <xml.h>

char* read_file(const char* filename, size_t* size_out) {
    if (!filename) return 0;
    FILE* f = fopen(filename, "r");
    if (!f) return 0;

    fseek(f, 0, SEEK_END);
    size_t total_size = ftell(f);
    rewind(f);
    if (total_size == 0) {
        fclose(f);
        return 0;
    }

    char* buf = malloc(total_size + 1);
    if (!buf) {
        fclose(f);
        return 0;
    }

    size_t read = fread(buf, 1, total_size, f);
    if (read < total_size) {
        fclose(f);
        return 0;
    }

    buf[read] = '\0';
    fclose(f);

    if (size_out) *size_out = read;

    return buf;
}

int main(void) {
    size_t len;
    const char* xml = read_file("../books.xml", &len);
    if (len == 0) {
        fprintf(stderr, "Error: could not read XML file.\n");
        return 1;
    }
    printf("%s\n", xml);
    return 0;
}
