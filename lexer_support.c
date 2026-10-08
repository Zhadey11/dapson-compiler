#include "lexer.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int lexical_errors = 0;

static char decode_escape(char code) {
  switch (code) {
  case 'n': return '\n';
  case 't': return '\t';
  case 'r': return '\r';
  case '0': return '\0';
  default: return code; /* \\  \"  \'  */
  }
}

/* Para literales de caracter: yytext llega como 'x' o '\n' */
char parse_char_escape(const char *str) {
  if (str[1] != '\\') {
    return str[1];
  }
  return decode_escape(str[2]);
}

/* Para literales de texto: recibe el contenido sin comillas */
char *unescape_string(const char *body) {
  char *result = malloc(strlen(body) + 1);
  if (result == NULL) {
    fprintf(stderr, "Sin memoria\n");
    exit(EXIT_FAILURE);
  }

  char *out = result;
  for (const char *in = body; *in != '\0'; in++) {
    if (*in == '\\' && in[1] != '\0') {
      in++;
      *out++ = decode_escape(*in);
    } else {
      *out++ = *in;
    }
  }
  *out = '\0';
  return result;
}

void lexical_error(int line, const char *message, const char *text) {
  lexical_errors++;
  fprintf(stderr, "Error lexico [linea %d]: %s '%s'\n", line, message, text);
}
