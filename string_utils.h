#ifndef STRING_UTILS_H
#define STRING_UTILS_H

#include <stddef.h>

char *string_copy(const char *value);

/* Reemplaza todas las apariciones de "from" por "to" (devuelve memoria nueva) */
char *string_replace(const char *source, const char *from, const char *to);

/* Ayudas para imprimir tablas alineadas (cuentan caracteres UTF-8) */
size_t utf8_length(const char *text);
void table_print_cell(const char *text, size_t width);
void table_print_rule(const size_t *widths, int columns);

#endif
