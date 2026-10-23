%{
#include "ast.h"
#include "parser.tab.h"
#include "string_utils.h"
#include "lexer.h"
#include "chess.h"          /* NUEVO */
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
"funcion"                     { return TOKEN_FUNCION; }
"retornar"                    { return TOKEN_RETORNAR; }
"nada"                        { return TOKEN_NADA; }
"si"                          { return TOKEN_SI; }
"sino"                        { return TOKEN_SINO; }
"mover"                       { return TOKEN_MOVER; }   /* NUEVO */
"hacia"                       { return TOKEN_HACIA; }   /* NUEVO */

"=="                          { return TOKEN_EQ; }
"!="                          { return TOKEN_NE; }
">="                          { return TOKEN_GE; }
"<="                          { return TOKEN_LE; }
">"                           { return TOKEN_GT; }
"<"                           { return TOKEN_LT; }
"||"                          { return TOKEN_OR; }
"es"                          { return TOKEN_EQ; }
"no"[ \t\r\n]+"es"             { return TOKEN_NE; }
"mayor"                       { return TOKEN_GT; }
"menor"                       { return TOKEN_LT; }
"o"                           { return TOKEN_OR; }

"entero"|"decimal"|"texto"|"caracter"|"booleano" {
                                yylval.str_val = string_copy(yytext);
                                return TOKEN_TYPE;
                              }

"verdadero"                   { yylval.bool_val = 1; return TOKEN_BOOL_LITERAL; }
"falso"                       { yylval.bool_val = 0; return TOKEN_BOOL_LITERAL; }

(peon_[bn][1-8])|((torre|caballo|alfil)_[bn][12])|((reina|rey)_[bn]) {
                                yylval.int_val = chess_piece_from_name(yytext);
                                return TOKEN_PIECE;
                              }
[a-h][1-8]                    { yylval.int_val = chess_square_from_name(yytext); return TOKEN_SQUARE; }

[:=;,.+\-*/(){}\[\]]          { return yytext[0]; }

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