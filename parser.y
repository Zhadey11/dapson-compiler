%{
#include "ast.h"
#include "string_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern int yylex(void);
void yyerror(const char *s);

ASTNode *ast_root = NULL; /* raiz del AST, la lee main.c */
int syntax_errors = 0;    /* cantidad de errores sintacticos */
%}

%locations
%define parse.error verbose

%union {
    long long int_val;
    double decimal_val;
    char *str_val;
    char char_val;
    int bool_val;
    struct ASTNode *node;
}

%token YYEOF 0 "fin de archivo"
%token TOKEN_SEA "sea"
%token TOKEN_FIJO "fijo"
%token TOKEN_IMPRIMIR "imprimir"
%token <str_val> TOKEN_TYPE "tipo de dato"
%token <str_val> TOKEN_IDENTIFIER "identificador"
%token <str_val> TOKEN_STRING_LITERAL "texto"
%token <int_val> TOKEN_INT_LITERAL "numero entero"
%token <decimal_val> TOKEN_DECIMAL_LITERAL "numero decimal"
%token <bool_val> TOKEN_BOOL_LITERAL "verdadero/falso"
%token <char_val> TOKEN_CHAR_LITERAL "caracter"

/* Precedencia: de menor a mayor */
%left '+' '-'
%left '*' '/'
%right UMINUS

%type <node> statement_list statement expression

/* Si el parser descarta un valor (por un error), se libera la memoria */
%destructor { free($$); } <str_val>
%destructor { free_ast($$); } <node>

%%

program:
    /* vacio */ {
        ast_root = NULL;
    }
    | statement_list {
        ast_root = $1;
    }
    ;

statement_list:
    statement {
        $$ = $1;
    }
    | statement_list statement {
        if ($2 == NULL) {
            $$ = $1;
        } else if ($1 == NULL) {
            $$ = $2;
        } else {
            ASTNode *cur = $1;
            while (cur->next != NULL) {
                cur = cur->next;
            }
            cur->next = $2;
            $$ = $1;
        }
    }
    ;

statement:
    /* Regla 1: declaracion de variable      sea nombre: tipo = valor; */
    TOKEN_SEA TOKEN_IDENTIFIER ':' TOKEN_TYPE '=' expression ';' {
        $$ = create_decl_node(NODE_VAR_DECL, $2, parse_type_name($4), $6, @1.first_line);
        free($2);
        free($4);
    }
    /* Declaracion de constante                fijo nombre: tipo = valor; */
    | TOKEN_FIJO TOKEN_IDENTIFIER ':' TOKEN_TYPE '=' expression ';' {
        $$ = create_decl_node(NODE_CONST_DECL, $2, parse_type_name($4), $6, @1.first_line);
        free($2);
        free($4);
    }
    /* Regla 2: asignacion de variable         nombre = valor; */
    | TOKEN_IDENTIFIER '=' expression ';' {
        $$ = create_assign_node($1, $3, @1.first_line);
        free($1);
    }
    /* Salida por pantalla                     imprimir expresion; */
    | TOKEN_IMPRIMIR expression ';' {
        $$ = create_print_node($2, @1.first_line);
    }
    /* Recuperacion de errores: se salta hasta el proximo ';' */
    | error ';' {
        yyerrok;
        $$ = NULL;
    }
    ;

expression:
    TOKEN_INT_LITERAL {
        $$ = create_int_node($1, @1.first_line);
    }
    | TOKEN_DECIMAL_LITERAL {
        $$ = create_decimal_node($1, @1.first_line);
    }
    | TOKEN_STRING_LITERAL {
        $$ = create_text_node($1, @1.first_line);
        free($1);
    }
    | TOKEN_CHAR_LITERAL {
        $$ = create_char_node($1, @1.first_line);
    }
    | TOKEN_BOOL_LITERAL {
        $$ = create_bool_node($1, @1.first_line);
    }
    | TOKEN_IDENTIFIER {
        $$ = create_identifier_node($1, @1.first_line);
        free($1);
    }
    | '(' expression ')' {
        $$ = $2;
    }
    | expression '+' expression {
        $$ = create_binary_node(OP_ADD, $1, $3, @2.first_line);
    }
    | expression '-' expression {
        $$ = create_binary_node(OP_SUB, $1, $3, @2.first_line);
    }
    | expression '*' expression {
        $$ = create_binary_node(OP_MUL, $1, $3, @2.first_line);
    }
    | expression '/' expression {
        $$ = create_binary_node(OP_DIV, $1, $3, @2.first_line);
    }
    | '-' expression %prec UMINUS {
        $$ = create_unary_node(OP_NEG, $2, @1.first_line);
    }
    ;

%%

void yyerror(const char *s) {
    /* Bison escribe el mensaje en ingles; se traduce lo basico */
    char *step1 = string_replace(s, "syntax error, unexpected", "token inesperado:");
    char *step2 = step1 ? string_replace(step1, ", expecting", ", se esperaba") : NULL;
    char *step3 = step2 ? string_replace(step2, " or ", " o ") : NULL;

    syntax_errors++;
    fprintf(stderr, "Error sintactico [linea %d]: %s\n", yylloc.first_line,
            step3 ? step3 : s);

    free(step1);
    free(step2);
    free(step3);
}
