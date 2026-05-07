#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *f = fopen("test_50mb.txt", "r");
    FILE *out = fopen("salida_clasica.txt", "w");
    if (!f || !out) { perror("fopen"); return 1; }
    int c;
    while ((c = fgetc(f)) != EOF)
        fputc(c, out);
    fclose(f);
    fclose(out);
    printf("Listo (metodo clasico)\n");
    return 0;
}