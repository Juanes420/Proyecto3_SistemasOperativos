/*
 * editor.h
 * Declara la estructura Gap Buffer para edición eficiente de texto.
 *
 * ¿Qué es un Gap Buffer?
 * Es un arreglo con un "hueco" (gap) en la posición del cursor.
 * Insertar/borrar en el cursor es O(1) porque solo mueves el gap.
 *
 * Ejemplo visual:
 *   Texto: "Hola mundo"  cursor después de "Hola"
 *   Buffer: [H][o][l][a][_][_][_][_][m][u][n][d][o]
 *                          ^gap_start  ^gap_end
 *
 * Insertar 'X': metes X al inicio del gap y avanzas gap_start
 *   Buffer: [H][o][l][a][X][_][_][_][m][u][n][d][o]
 */

#ifndef EDITOR_H
#define EDITOR_H

#include <stddef.h>   /* size_t */

#define GAP_DEFAULT_SIZE  256   /* tamaño inicial del gap en bytes */
#define BUFFER_GROW_SIZE  512   /* cuánto crece el buffer cuando se llena */

typedef struct {
    char   *data;       /* puntero al buffer completo en heap (malloc) */
    size_t  gap_start;  /* índice donde empieza el gap (posición cursor) */
    size_t  gap_end;    /* índice donde termina el gap (exclusive) */
    size_t  capacity;   /* tamaño total del buffer en bytes */
} GapBuffer;

/* --- Funciones del editor --- */

/* Crea un nuevo GapBuffer vacío. Retorna NULL si falla malloc. */
GapBuffer *gb_create(void);

/* Libera toda la memoria del GapBuffer. Siempre llama esto al salir. */
void gb_destroy(GapBuffer *gb);

/* Inserta el caracter c en la posición actual del cursor. */
int gb_insert(GapBuffer *gb, char c);

/* Borra el caracter anterior al cursor (como Backspace). */
int gb_delete(GapBuffer *gb);

/* Mueve el cursor a la posición pos (0-indexed). */
int gb_move_cursor(GapBuffer *gb, size_t pos);

/* Retorna cuántos caracteres de texto real hay (sin contar el gap). */
size_t gb_length(const GapBuffer *gb);

/*
 * Copia el texto real (sin el gap) a un buffer contiguo.
 * El llamador debe hacer free() del puntero retornado.
 * Retorna NULL si falla malloc.
 */
char *gb_get_text(const GapBuffer *gb);

#endif /* EDITOR_H */