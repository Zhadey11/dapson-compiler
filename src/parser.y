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

/* Agrega "node" al final de una lista enlazada (con next). Tolera NULL. */
static ASTNode *append_node(ASTNode *list, ASTNode *node) {
    if (node == NULL) {
        return list;
    }
    if (list == NULL) {
        return node;
    }
    ASTNode *cur = list;
    while (cur->next != NULL) {
        cur = cur->next;
    }
    cur->next = node;
    return list;
}
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
%token TOKEN_FUNCION "funcion"
%token TOKEN_RETORNAR "retornar"
%token TOKEN_NADA "nada"
%token TOKEN_SI "si"
%token TOKEN_SINO "sino"
%token TOKEN_EQ "== (es)"
%token TOKEN_NE "!= (no es)"
%token TOKEN_GT "> (mayor)"
%token TOKEN_LT "< (menor)"
%token TOKEN_GE ">="
%token TOKEN_LE "<="
%token TOKEN_OR "|| (o)"
%token <str_val> TOKEN_TYPE "tipo de dato"
%token <str_val> TOKEN_IDENTIFIER "identificador"
%token <str_val> TOKEN_STRING_LITERAL "texto"
%token <int_val> TOKEN_INT_LITERAL "numero entero"
%token <decimal_val> TOKEN_DECIMAL_LITERAL "numero decimal"
%token <bool_val> TOKEN_BOOL_LITERAL "verdadero/falso"
%token <char_val> TOKEN_CHAR_LITERAL "caracter"

/* Precedencia: de menor a mayor */
%left TOKEN_OR
%nonassoc TOKEN_EQ TOKEN_NE TOKEN_GT TOKEN_LT TOKEN_GE TOKEN_LE
%left '+' '-'
%left '*' '/'
%right UMINUS

%type <node> item_list item function_def
%type <node> statement_list statement_list_opt statement expression
%type <node> param_list param_list_opt param arg_list arg_list_opt
%type <node> if_statement block else_part
%type <int_val> return_type

/* Si el parser descarta un valor (por un error), se libera la memoria */
%destructor { free($$); } <str_val>
%destructor { free_ast($$); } <node>

%%

program:
    /* vacio */ {
        ast_root = NULL;
    }
    | item_list {
        ast_root = $1;
    }
    ;

/* Nivel superior: sentencias y definiciones de funciones */
item_list:
    item {
        $$ = $1;
    }
    | item_list item {
        $$ = append_node($1, $2);
    }
    ;

item:
    statement {
        $$ = $1;
    }
    | function_def {
        $$ = $1;
    }
    ;

/* Regla 3: funciones     funcion nombre(a: tipo, b: tipo): tipo { ... } */
function_def:
    TOKEN_FUNCION TOKEN_IDENTIFIER '(' param_list_opt ')' return_type
    '{' statement_list_opt '}' {
        $$ = create_func_node($2, $4, (DataType)$6, $8, @1.first_line);
        free($2);
    }
    ;

/* ": nada" (o sin nada) = la funcion no devuelve un valor */
return_type:
    /* vacio */ {
        $$ = TYPE_NADA;
    }
    | ':' TOKEN_NADA {
        $$ = TYPE_NADA;
    }
    | ':' TOKEN_TYPE {
        $$ = parse_type_name($2);
        free($2);
    }
    ;

param_list_opt:
    /* vacio */ {
        $$ = NULL;
    }
    | param_list {
        $$ = $1;
    }
    ;

param_list:
    param {
        $$ = $1;
    }
    | param_list ',' param {
        $$ = append_node($1, $3);
    }
    ;

param:
    TOKEN_IDENTIFIER ':' TOKEN_TYPE {
        $$ = create_param_node($1, parse_type_name($3), @1.first_line);
        free($1);
        free($3);
    }
    ;

/* Sentencias (dentro de una funcion no se pueden definir otras funciones) */
statement_list_opt:
    /* vacio */ {
        $$ = NULL;
    }
    | statement_list {
        $$ = $1;
    }
    ;

statement_list:
    statement {
        $$ = $1;
    }
    | statement_list statement {
        $$ = append_node($1, $2);
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
    /* Llamada a una funcion como sentencia    nombre(argumentos); */
    | TOKEN_IDENTIFIER '(' arg_list_opt ')' ';' {
        $$ = create_call_node($1, $3, @1.first_line);
        free($1);
    }
    /* Retorno de una funcion                  retornar expresion; */
    | TOKEN_RETORNAR expression ';' {
        $$ = create_return_node($2, @1.first_line);
    }
    | TOKEN_RETORNAR ';' {
        $$ = create_return_node(NULL, @1.first_line);
    }
    /* Condicional                            si condicion: { ... } sino { ... } */
    | if_statement {
        $$ = $1;
    }
    /* Recuperacion de errores: se salta hasta el proximo ';' */
    | error ';' {
        yyerrok;
        $$ = NULL;
    }
    ;

/* Regla 4: condicional */
if_statement:
    TOKEN_SI expression ':' block else_part {
        $$ = create_if_node($2, $4, $5, @1.first_line);
    }
    ;

block:
    '{' statement_list_opt '}' {
        $$ = create_block_node($2, @1.first_line);
    }
    ;

else_part:
    /* vacio */ {
        $$ = NULL;
    }
    | TOKEN_SINO block {
        $2->data.block.is_else = 1;
        $$ = $2;
    }
    ;

arg_list_opt:
    /* vacio */ {
        $$ = NULL;
    }
    | arg_list {
        $$ = $1;
    }
    ;

arg_list:
    expression {
        $$ = $1;
    }
    | arg_list ',' expression {
        $$ = append_node($1, $3);
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
    | TOKEN_IDENTIFIER '(' arg_list_opt ')' {
        $$ = create_call_node($1, $3, @1.first_line);
        free($1);
    }
    | '(' expression ')' {
        $$ = $2;
    }
    | expression TOKEN_OR expression {
        $$ = create_binary_node(OP_OR, $1, $3, @2.first_line);
    }
    | expression TOKEN_EQ expression {
        $$ = create_binary_node(OP_EQ, $1, $3, @2.first_line);
    }
    | expression TOKEN_NE expression {
        $$ = create_binary_node(OP_NE, $1, $3, @2.first_line);
    }
    | expression TOKEN_GT expression {
        $$ = create_binary_node(OP_GT, $1, $3, @2.first_line);
    }
    | expression TOKEN_LT expression {
        $$ = create_binary_node(OP_LT, $1, $3, @2.first_line);
    }
    | expression TOKEN_GE expression {
        $$ = create_binary_node(OP_GE, $1, $3, @2.first_line);
    }
    | expression TOKEN_LE expression {
        $$ = create_binary_node(OP_LE, $1, $3, @2.first_line);
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