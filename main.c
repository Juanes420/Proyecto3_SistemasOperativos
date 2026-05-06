#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#include <zlib.h>
#include "editor.h"
#include "compress.h"
#include "io.h"
#include "format.h"

static int save_file(GapBuffer *gb, const char *path)
{
    char *text = gb_get_text(gb);
    if (!text) return -1;
    size_t text_len = strlen(text);

    char  *compressed = NULL;
    size_t comp_size  = 0;
    if (compress_data(text, text_len, &compressed, &comp_size) != 0) {
        free(text); return -1;
    }
    free(text);

    FileHeader hdr;
    memset(&hdr, 0, sizeof(hdr));
    memcpy(hdr.magic, CEZ_MAGIC, 3);
    hdr.version         = CEZ_VERSION;
    hdr.flags           = FLAG_COMPRESSED;
    hdr.header_size     = (uint16_t)HEADER_SIZE;
    hdr.original_size   = (uint32_t)text_len;
    hdr.compressed_size = (uint32_t)comp_size;
    hdr.timestamp       = (uint64_t)time(NULL);
    snprintf(hdr.filename, sizeof(hdr.filename), "%s", path);

    /* Calcular CRC32 del payload comprimido usando zlib */
    hdr.crc32_checksum  = (uint32_t)crc32(0L, (const Bytef *)compressed, (uInt)comp_size);

    size_t total_size = sizeof(FileHeader) + comp_size;
    char *final_buf = (char *)malloc(total_size);
    if (!final_buf) { free(compressed); return -1; }
    memcpy(final_buf, &hdr, sizeof(FileHeader));
    memcpy(final_buf + sizeof(FileHeader), compressed, comp_size);
    free(compressed);

    int ret = io_write_file(path, final_buf, total_size);
    free(final_buf);
    if (ret == 0)
        printf("Guardado: %s (%zu bytes en disco, original: %u bytes)\n",
               path, total_size, hdr.original_size);
    return ret;
}

static int open_file(GapBuffer *gb, const char *path)
{
    char  *raw = NULL;
    size_t raw_size = 0;
    if (io_read_file(path, &raw, &raw_size) != 0) return -1;

    if (raw_size < sizeof(FileHeader) || memcmp(raw, CEZ_MAGIC, 3) != 0) {
        fprintf(stderr, "Archivo CEZ inválido\n");
        free(raw); return -1;
    }

    FileHeader hdr;
    memcpy(&hdr, raw, sizeof(FileHeader));

    char  *payload = raw + sizeof(FileHeader);
    size_t pay_sz  = raw_size - sizeof(FileHeader);

    /* Verificar CRC32 antes de descomprimir */
    uint32_t computed_crc = (uint32_t)crc32(0L, (const Bytef *)payload, (uInt)pay_sz);
    if (computed_crc != hdr.crc32_checksum) {
        fprintf(stderr, "Error: CRC32 no coincide — archivo corrupto\n");
        free(raw); return -1;
    }

    char  *text = NULL;
    if (decompress_data(payload, pay_sz, &text, hdr.original_size) != 0) {
        free(raw); return -1;
    }
    free(raw);

    for (size_t i = 0; i < strlen(text); i++)
        gb_insert(gb, text[i]);
    free(text);

    printf("Abierto: %s (%u bytes)\n", path, hdr.original_size);
    return 0;
}

static void display(const GapBuffer *gb)
{
    char *text = gb_get_text(gb);
    if (!text) return;
    printf("\n--- Contenido (%zu chars) ---\n%s\n---\n", strlen(text), text);
    free(text);
}

int main(void)
{
    GapBuffer *gb = gb_create();
    if (!gb) { fprintf(stderr, "Error creando editor\n"); return 1; }

    char filename[256] = "documento.cez";
    int  running = 1;

    printf("=== Editor CEZ ===\nArchivo: %s\n", filename);

    while (running) {
        printf("\n[1]Escribir [2]Borrar [3]Ver [4]Guardar [5]Abrir [6]Renombrar [0]Salir\nOpción: ");
        int op;
        if (scanf("%d", &op) != 1) break;
        getchar();

        switch (op) {
        case 1: {
            printf("Texto (Enter para terminar):\n");
            char line[1024];
            if (fgets(line, sizeof(line), stdin))
                for (size_t i = 0; i < strlen(line); i++)
                    gb_insert(gb, line[i]);
            break;
        }
        case 2:
            printf(gb_delete(gb) == 0 ? "Borrado.\n" : "Nada que borrar.\n");
            break;
        case 3: display(gb); break;
        case 4: save_file(gb, filename); break;
        case 5: {
            gb_destroy(gb);
            gb = gb_create();
            if (!gb) {
                fprintf(stderr, "Error: no se pudo crear el editor.\n");
                running = 0;
                break;
            }
            open_file(gb, filename);
            break;
        }
        case 6: {
            printf("Nuevo nombre: ");
            char tmp[256];
            if (fgets(tmp, sizeof(tmp), stdin)) {
                tmp[strcspn(tmp, "\n")] = '\0';
                snprintf(filename, sizeof(filename), "%s", tmp);
            }
            break;
        }
        case 0: running = 0; break;
        default: printf("Opción inválida.\n");
        }
    }

    gb_destroy(gb);
    printf("saliendo\n");
    return 0;
}