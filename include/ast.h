#ifndef AST_H
#define AST_H

/* Tipos de dato de Dapson */
typedef enum {
  TYPE_ENTERO,
  TYPE_DECIMAL,
  TYPE_TEXTO,
  TYPE_CARACTER,
  TYPE_BOOLEANO,
  /* Arreglos: el ORDEN importa (el mismo que los 5 tipos de arriba) */
  TYPE_ENTERO_ARR,
  TYPE_DECIMAL_ARR,
  TYPE_TEXTO_ARR,
  TYPE_CARACTER_ARR,
  TYPE_BOOLEANO_ARR,
  TYPE_NADA,    /* una funcion que no devuelve valor */
  TYPE_UNKNOWN
} DataType;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
  OP_EQ, /* ==  (o "es")          */
  OP_NE, /* !=  (o "no es")       */
  OP_GT, /* >   (o "mayor")       */
  OP_LT, /* <   (o "menor")       */
  OP_GE, /* >=                    */
  OP_LE, /* <=                    */
  OP_OR, /* ||  (o "o")           */
  OP_NEG /* menos unario: -5 */
} Operator;

typedef enum {
  NODE_VAR_DECL,   /* sea nombre: tipo = valor;   */
  NODE_CONST_DECL, /* fijo nombre: tipo = valor;  */
  NODE_ASSIGN,     /* nombre = valor;             */
  NODE_PRINT,      /* imprimir expresion;         */
  NODE_INT,
  NODE_DECIMAL,
  NODE_TEXT,
  NODE_CHAR,
  NODE_BOOL,
  NODE_IDENT,
  NODE_BINARY,
  NODE_UNARY,
  NODE_FUNC_DECL, /* funcion nombre(params): tipo { ... } */
  NODE_PARAM,     /* parametro de una funcion: nombre: tipo */
  NODE_CALL,      /* nombre(argumentos)          */
  NODE_RETURN,    /* retornar expresion;         */
  NODE_IF,        /* si condicion: { ... } sino { ... } */
  NODE_BLOCK,     /* las sentencias entre { } de un si o un sino */
  NODE_ARRAY,     /* [10, 20, 30]                */
  NODE_INDEX,     /* lista[i]                    */
  NODE_ARRAY_CALL, /* lista.agregar(x)  lista.quitar()  lista.largo() */
  NODE_MOVE        /* mover peon_b5 hacia e4;     (NUEVO) */
} NodeType;

/* Los metodos de un arreglo */
typedef enum {
  ARRAY_AGREGAR,    /* lista.agregar(x): pone x al final            */
  ARRAY_QUITAR,     /* lista.quitar():   saca y devuelve el ultimo  */
  ARRAY_LARGO,      /* lista.largo():    cantidad de elementos      */
  ARRAY_DESCONOCIDO /* un nombre que no es ninguno de los tres      */
} ArrayMethod;

typedef struct ASTNode {
  NodeType type;
  DataType exprType; /* lo completa el analizador semantico */
  int line;          /* linea del codigo fuente */
  union {
    long long intValue;
    double decimalValue;
    char *strValue;
    char charValue;
    int boolValue;
    char *identifier;
    struct {
      char *name;
      struct ASTNode *init;
    } decl;
    struct {
      char *name;
      struct ASTNode *value;
    } assign;
    struct {
      struct ASTNode *value;
    } print;
    struct {
      Operator op;
      struct ASTNode *left;
      struct ASTNode *right; /* NULL en el menos unario */
    } binary;
    struct {
      char *name;
      struct ASTNode *params; /* lista de NODE_PARAM (enlazada con next) */
      struct ASTNode *body;   /* lista de sentencias                     */
    } func;                   /* el tipo de retorno va en exprType       */
    struct {
      char *name;
      struct ASTNode *args; /* lista de expresiones (enlazada con next) */
    } call;
    struct {
      struct ASTNode *value; /* NULL en "retornar;" */
    } ret;
    struct {
      struct ASTNode *cond;       /* la condicion (debe ser booleano)   */
      struct ASTNode *then_block; /* NODE_BLOCK del "si"                */
      struct ASTNode *else_block; /* NODE_BLOCK del "sino" (o NULL)     */
    } if_stmt;
    struct {
      struct ASTNode *body; /* lista de sentencias (enlazada con next) */
      int is_else;          /* 1 si es el bloque del "sino"            */
    } block;
    struct {
      struct ASTNode *elements; /* lista de expresiones (enlazada con next) */
    } array;
    struct {
      char *name;              /* el arreglo                  */
      struct ASTNode *index;   /* la posicion (debe ser entero) */
    } index;
    struct {
      char *name;              /* el arreglo                  */
      char *method_name;       /* el nombre tal como se escribio */
      ArrayMethod method;
      struct ASTNode *args;    /* lista de expresiones (enlazada con next) */
    } array_call;
    struct {
      int piece;  /* 0..31, ver chess.h */
      int square; /* 0..63, ver chess.h */
    } move;       /* NUEVO */
  } data;

  struct ASTNode *next; /* lista enlazada de sentencias */
} ASTNode;

ASTNode *create_int_node(long long val, int line);
ASTNode *create_decimal_node(double val, int line);
ASTNode *create_text_node(const char *val, int line);
ASTNode *create_char_node(char val, int line);
ASTNode *create_bool_node(int val, int line);
ASTNode *create_identifier_node(const char *name, int line);
ASTNode *create_binary_node(Operator op, ASTNode *left, ASTNode *right,
                            int line);
ASTNode *create_unary_node(Operator op, ASTNode *operand, int line);
ASTNode *create_decl_node(NodeType kind, const char *name, DataType type,
                          ASTNode *init, int line);
ASTNode *create_assign_node(const char *name, ASTNode *value, int line);
ASTNode *create_print_node(ASTNode *value, int line);
ASTNode *create_func_node(const char *name, ASTNode *params, DataType ret_type,
                          ASTNode *body, int line);
ASTNode *create_param_node(const char *name, DataType type, int line);
ASTNode *create_call_node(const char *name, ASTNode *args, int line);
ASTNode *create_return_node(ASTNode *value, int line);
ASTNode *create_block_node(ASTNode *body, int line);
ASTNode *create_if_node(ASTNode *cond, ASTNode *then_block,
                        ASTNode *else_block, int line);

ASTNode *create_array_node(ASTNode *elements, int line);
ASTNode *create_index_node(const char *name, ASTNode *index, int line);
ASTNode *create_array_call_node(const char *name, const char *method_name,
                                ASTNode *args, int line);

ASTNode *create_move_node(int piece, int square, int line); /* NUEVO */

int ast_list_length(const ASTNode *list); /* cuenta nodos enlazados con next */

DataType parse_type_name(const char *name);
int is_array_type(DataType type);
DataType array_type_of(DataType element_type); /* entero -> entero[] */
DataType element_type_of(DataType array_type); /* entero[] -> entero */
const char *type_name(DataType type);
const char *operator_symbol(Operator op);

void print_ast_tree(const ASTNode *root);  /* arbol con indentacion */
void print_ast_table(const ASTNode *root); /* tabla de nodos        */
void free_ast(ASTNode *node);

#endif