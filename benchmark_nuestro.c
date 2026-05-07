#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "compress.h"
#include "io.h"

int main(void) {
    char  *input = NULL;
    size_t input_size = 0;
    if (io_read_file("test_50mb.txt", &input, &input_size) != 0) return 1;
    char  *compressed = NULL;
    size_t comp_size  = 0;
    if (compress_data(input, input_size, &compressed, &comp_size) != 0) {
        free(input); return 1;
    }
    free(input);
    io_write_file("salida_nuestro.cez", compressed, comp_size);
    free(compressed);
    printf("Listo (metodo nuestro)\n");
    return 0;
}