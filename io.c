/*
 * io.c
 * I/O de bajo nivel con syscalls POSIX directas.
 * Escribe en bloques de PAGE_SIZE (4096 bytes) para minimizar
 * la cantidad de llamadas write() y por ende los context switches.
 */

#include "io.h"
#include <fcntl.h>      /* open(), O_WRONLY, O_CREAT, O_TRUNC, O_RDONLY */
#include <unistd.h>     /* write(), read(), close() */
#include <sys/stat.h>   /* fstat(), struct stat */
#include <stdlib.h>     /* malloc, free */
#include <stdio.h>      /* perror */
#include <string.h>     /* memcpy */

/*
 * io_write_file: escribe 'size' bytes de 'data' en 'path'.
 *
 * Estrategia de bloques de 4KB:
 *   En lugar de llamar write(fd, data, size) una sola vez con todo,
 *   usamos un buffer intermedio de PAGE_SIZE y llamamos write() en
 *   múltiplos del tamaño de página del kernel.
 *
 *   Esto reduce la fragmentación del bus I/O y alinea las escrituras
 *   con el sistema de caché del kernel (page cache).
 *
 * Flags de open():
 *   O_WRONLY  = abrir solo para escritura
 *   O_CREAT   = crear si no existe
 *   O_TRUNC   = truncar a 0 si existe
 *   0644      = permisos rw-r--r--
 */
int io_write_file(const char *path, const char *data, size_t size)
{
    int fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) {
        perror("open write");
        return -1;
    }

    size_t written = 0;
    while (written < size) {
        /* Calcular cuántos bytes quedan, máximo PAGE_SIZE por llamada */
        size_t chunk = size - written;
        if (chunk > PAGE_SIZE) chunk = PAGE_SIZE;

        ssize_t ret = write(fd, data + written, chunk);
        if (ret < 0) {
            perror("write");
            close(fd);
            return -1;
        }
        written += (size_t)ret;
    }

    close(fd);
    return 0;
}

/*
 * io_read_file: lee el archivo 'path' completo.
 *
 * Pasos:
 * 1. Abrir con open() — obtenemos un file descriptor (fd)
 * 2. Usar fstat() para conocer el tamaño sin leer todo primero
 * 3. malloc exactamente ese tamaño
 * 4. Leer en bloques de PAGE_SIZE con read()
 * 5. close(fd)
 */
int io_read_file(const char *path, char **output, size_t *out_size)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        perror("open read");
        return -1;
    }

    /* fstat: obtiene metadatos del archivo sin leerlo */
    struct stat st;
    if (fstat(fd, &st) < 0) {
        perror("fstat");
        close(fd);
        return -1;
    }

    size_t file_size = (size_t)st.st_size;

    *output = (char *)malloc(file_size);
    if (!*output) {
        perror("read malloc");
        close(fd);
        return -1;
    }

    size_t total_read = 0;
    while (total_read < file_size) {
        size_t chunk = file_size - total_read;
        if (chunk > PAGE_SIZE) chunk = PAGE_SIZE;

        ssize_t ret = read(fd, *output + total_read, chunk);
        if (ret < 0) {
            perror("read");
            free(*output);
            *output = NULL;
            close(fd);
            return -1;
        }
        if (ret == 0) break; /* EOF inesperado */
        total_read += (size_t)ret;
    }

    *out_size = total_read;
    close(fd);
    return 0;
}