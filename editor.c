/*
 * editor.c
 * Implementación del Gap Buffer para edición eficiente de texto.
 */

#include "editor.h"
#include <stdlib.h>   /* malloc, realloc, free */
#include <string.h>   /* memmove, memset */
#include <stdio.h>    /* perror */

/*
 * gb_create: reserva memoria para un GapBuffer vacío.
 * Estado inicial:
 *   [_][_][_]...[_]   (todo es gap al principio)
 *    ^gap_start=0
 *                ^gap_end=GAP_DEFAULT_SIZE
 */
GapBuffer *gb_create(void)
{
    GapBuffer *gb = (GapBuffer *)malloc(sizeof(GapBuffer));
    if (!gb) { perror("gb_create malloc GapBuffer"); return NULL; }

    gb->data = (char *)calloc(GAP_DEFAULT_SIZE, sizeof(char));
    if (!gb->data) {
        perror("gb_create calloc data");
        free(gb);
        return NULL;
    }

    gb->gap_start = 0;
    gb->gap_end   = GAP_DEFAULT_SIZE;
    gb->capacity  = GAP_DEFAULT_SIZE;
    return gb;
}

/*
 * gb_destroy: libera toda la memoria.
 * SIEMPRE debe llamarse antes de salir para evitar memory leaks.
 */
void gb_destroy(GapBuffer *gb)
{
    if (!gb) return;
    free(gb->data);   /* primero el buffer interno */
    free(gb);         /* luego el struct */
}

/*
 * gb_grow: función interna que agranda el buffer cuando el gap se agota.
 * Usa realloc para redimensionar sin copiar manualmente.
 */
static int gb_grow(GapBuffer *gb)
{
    size_t new_capacity = gb->capacity + BUFFER_GROW_SIZE;
    char *new_data = (char *)realloc(gb->data, new_capacity);
    if (!new_data) { perror("gb_grow realloc"); return -1; }

    /*
     * Después de crecer, necesitamos mover la parte derecha del texto
     * para que el gap sea contiguo en el medio.
     * Antes: [texto_izq][gap_viejo][texto_der]
     * Después:[texto_izq][gap_nuevo_grande][texto_der]
     */
    size_t right_len = gb->capacity - gb->gap_end;
    memmove(new_data + new_capacity - right_len,
            new_data + gb->gap_end,
            right_len);

    gb->data      = new_data;
    gb->gap_end   = new_capacity - right_len;
    gb->capacity  = new_capacity;
    return 0;
}

/*
 * gb_insert: inserta el caracter c en la posición del cursor (gap_start).
 * Complejidad: O(1) si hay espacio en el gap, O(n) si hay que crecer.
 */
int gb_insert(GapBuffer *gb, char c)
{
    /* Si el gap se agotó, crecer */
    if (gb->gap_start == gb->gap_end) {
        if (gb_grow(gb) != 0) return -1;
    }
    /* Colocar el caracter y avanzar gap_start */
    gb->data[gb->gap_start] = c;
    gb->gap_start++;
    return 0;
}

/*
 * gb_delete: elimina el caracter anterior al cursor (Backspace).
 * Simplemente retrocede gap_start — el caracter queda en el gap y
 * será sobreescrito en la próxima inserción.
 */
int gb_delete(GapBuffer *gb)
{
    if (gb->gap_start == 0) return -1; /* nada que borrar */
    gb->gap_start--;
    gb->data[gb->gap_start] = '\0'; /* limpiar por seguridad */
    return 0;
}

/*
 * gb_move_cursor: mueve el cursor a la posición 'pos'.
 * Mueve bytes del texto al otro lado del gap según la dirección.
 */
int gb_move_cursor(GapBuffer *gb, size_t pos)
{
    size_t current = gb->gap_start;
    size_t text_len = gb_length(gb);
    if (pos > text_len) pos = text_len;

    if (pos < current) {
        /* Mover cursor a la izquierda: mover texto de izq a der del gap */
        size_t diff = current - pos;
        gb->gap_end   -= diff;
        gb->gap_start -= diff;
        memmove(gb->data + gb->gap_end,
                gb->data + gb->gap_start,
                diff);
    } else if (pos > current) {
        /* Mover cursor a la derecha */
        size_t diff = pos - current;
        memmove(gb->data + gb->gap_start,
                gb->data + gb->gap_end,
                diff);
        gb->gap_start += diff;
        gb->gap_end   += diff;
    }
    return 0;
}

/*
 * gb_length: retorna el número de caracteres reales (sin el gap).
 */
size_t gb_length(const GapBuffer *gb)
{
    return gb->capacity - (gb->gap_end - gb->gap_start);
}

/*
 * gb_get_text: retorna una copia contigua del texto (sin el gap).
 * El llamador debe hacer free() del resultado.
 */
char *gb_get_text(const GapBuffer *gb)
{
    size_t len = gb_length(gb);
    /* +1 para el terminador nulo '\0' */
    char *text = (char *)malloc(len + 1);
    if (!text) { perror("gb_get_text malloc"); return NULL; }

    /* Copiar parte izquierda (antes del gap) */
    memcpy(text, gb->data, gb->gap_start);

    /* Copiar parte derecha (después del gap) */
    size_t right_len = gb->capacity - gb->gap_end;
    memcpy(text + gb->gap_start, gb->data + gb->gap_end, right_len);

    /* Terminar la string con nulo explícito */
    text[len] = '\0';
    return text;
}