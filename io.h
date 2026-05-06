/*
 * io.h
 * I/O de bajo nivel usando syscalls POSIX directas: open, read, write, close.
 * NO usamos fopen/fprintf — esas son abstracciones de stdio.h que agregan
 * una capa extra y buffers internos que no controlamos.
 *
 * Usamos bloques de PAGE_SIZE (4096 bytes = 4KB) porque:
 *   - El kernel maneja memoria en páginas de 4KB
 *   - Escribir en múltiplos de PAGE_SIZE minimiza las syscalls write()
 *   - Menos syscalls = menos context switches User↔Kernel = menos latencia
 */

#ifndef IO_H
#define IO_H

#include <stddef.h>   /* size_t */

#define PAGE_SIZE  4096   /* Tamaño de página del kernel Linux (x86-64) */

/*
 * Guarda 'size' bytes de 'data' en el archivo 'path'.
 * Internamente escribe en bloques de PAGE_SIZE.
 * Usa: open(O_WRONLY|O_CREAT|O_TRUNC) + write() + close()
 *
 * Retorna  0 en éxito.
 * Retorna -1 en error (imprime errno).
 */
int io_write_file(const char *path, const char *data, size_t size);

/*
 * Lee el archivo 'path' completo.
 * Escribe el contenido en '*output' (malloc interno).
 * Escribe el tamaño leído en '*out_size'.
 *
 * Retorna  0 en éxito.
 * Retorna -1 en error.
 * El llamador debe hacer free(*output).
 */
int io_read_file(const char *path, char **output, size_t *out_size);

#endif /* IO_H */