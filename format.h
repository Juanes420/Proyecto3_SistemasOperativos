/*
 * format.h
 * Define el formato binario del archivo .cez (Compressed Editor Zone)
 *
 * Estructura en disco:
 * [0..63]   → FileHeader  (magic number + metadatos)
 * [64..]    → payload comprimido con zlib
 */

#ifndef FORMAT_H
#define FORMAT_H

#include <stdint.h>   /* uint32_t, uint64_t — tamaños exactos, sin sorpresas */

/*
 * __attribute__((packed)) le dice al compilador:
 * "NO agregues bytes de padding entre campos"
 * Sin esto, el compilador alinea los campos y el struct ocupa más bytes
 * de lo esperado, rompiendo la lectura del archivo binario.
 *
 * Ejemplo SIN packed:
 *   char magic[4]   → 4 bytes
 *   [padding]       → 4 bytes extra que el compilador agrega solo
 *   uint32_t size   → 4 bytes
 *   Total: 12 bytes  (BAD)
 *
 * Ejemplo CON packed:
 *   char magic[4]   → 4 bytes
 *   uint32_t size   → 4 bytes  (inmediatamente después)
 *   Total: 8 bytes   (GOOD)
 */

typedef struct __attribute__((packed)) {
    char     magic[4];          /* Bytes 0-3:  "CEZ\0" identifica nuestro formato  */
    uint8_t  version;           /* Byte  4:    versión del formato (actualmente 1) */
    uint8_t  flags;             /* Byte  5:    bits de opciones (ej. bit0=tiene RTF)*/
    uint16_t header_size;       /* Bytes 6-7:  tamaño total de este header = 64    */
    uint32_t original_size;     /* Bytes 8-11: tamaño del texto ANTES de comprimir */
    uint32_t compressed_size;   /* Bytes 12-15: tamaño del payload comprimido      */
    uint32_t crc32_checksum;    /* Bytes 16-19: checksum para verificar integridad */
    uint64_t timestamp;         /* Bytes 20-27: Unix timestamp de última edición   */
    char     filename[28];      /* Bytes 28-55: nombre original del archivo        */
    uint8_t  reserved[8];       /* Bytes 56-63: reservado para uso futuro = {0}    */
} FileHeader;                   /* Total: exactamente 64 bytes                     */

/* Magic number que identifica nuestro formato */
#define CEZ_MAGIC       "CEZ"
#define CEZ_VERSION     1
#define HEADER_SIZE     64      /* sizeof(FileHeader) debe ser igual a esto        */

/* Flags del campo flags */
#define FLAG_RICH_TEXT  0x01    /* bit 0: el archivo contiene texto enriquecido   */
#define FLAG_COMPRESSED 0x02    /* bit 1: el payload está comprimido              */

/* Verificación en tiempo de compilación: el header DEBE ser exactamente 64 bytes.
 * Si esto falla, significa que el compilador agregó padding a pesar del packed. */
_Static_assert(sizeof(FileHeader) == HEADER_SIZE,
               "FileHeader debe ser exactamente 64 bytes — verificar __attribute__((packed))");

#endif /* FORMAT_H */