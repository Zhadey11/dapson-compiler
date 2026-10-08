%{
#include "ast.h"
#include "parser.tab.h"
#include "string_utils.h"
#include "lexer.h"
#include <string.h>
#include <stdlib.h>

/* Antes de cada regla: guardar la linea actual para los mensajes de error */
#define YY_USER_ACTION yylloc.first_line = yylloc.last_line = yylineno;
%}

%option noyywrap
%option yylineno

%%

[ \t\r\n]+                    ;                         /* espacios        */
"//".*                        ;                         /* comentario      */
"/*"([^*]|\*+[^*/])*\*+"/"    ;                         /* comentario de bloque */

"sea"                         { return TOKEN_SEA; }
"fijo"                        { return TOKEN_FIJO; }
"imprimir"                    { return TOKEN_IMPRIMIR; }

"entero"|"decimal"|"texto"|"caracter"|"booleano" {
                                yylval.str_val = string_copy(yytext);
                                return TOKEN_TYPE;
                              }

"verdadero"                   { yylval.bool_val = 1; return TOKEN_BOOL_LITERAL; }
"falso"                       { yylval.bool_val = 0; return TOKEN_BOOL_LITERAL; }

[:=;+\-*/()]                  { return yytext[0]; }

\"([^\\\"\n]|\\.)*\"          {
                                yytext[yyleng - 1] = '\0';   /* quita la comilla final */
                                yylval.str_val = unescape_string(yytext + 1);
                                return TOKEN_STRING_LITERAL;
                              }
\"[^\"\n]*                    { lexical_error(yylineno, "cadena sin cerrar", yytext); }

\'([^'\\\n]|\\.)\'            {
                                yylval.char_val = parse_char_escape(yytext);
                                return TOKEN_CHAR_LITERAL;
                              }

[0-9]+\.[0-9]+                { yylval.decimal_val = atof(yytext); return TOKEN_DECIMAL_LITERAL; }
[0-9]+                        { yylval.int_val = strtoll(yytext, NULL, 10); return TOKEN_INT_LITERAL; }
[a-zA-Z_][a-zA-Z0-9_]*        { yylval.str_val = string_copy(yytext); return TOKEN_IDENTIFIER; }

.                             { lexical_error(yylineno, "caracter no reconocido", yytext); }

%%
