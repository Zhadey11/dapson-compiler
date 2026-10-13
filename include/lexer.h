#ifndef LEX_H
#define LEX_H

extern int lexical_errors; /* cantidad de errores lexicos encontrados */

char parse_char_escape(const char *str);
char *unescape_string(const char *body);
void lexical_error(int line, const char *message, const char *text);

#endif
