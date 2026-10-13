#ifndef AST_H
#define AST_H

/* Tipos de dato de Dapson */
typedef enum {
  TYPE_ENTERO,
  TYPE_DECIMAL,
  TYPE_TEXTO,
  TYPE_CARACTER,
  TYPE_BOOLEANO,
  TYPE_NADA,
  TYPE_UNKNOWN
} DataType;

typedef enum {
  OP_ADD,
  OP_SUB,
  OP_MUL,
  OP_DIV,
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
  NODE_FUNC_DECL,
  NODE_PARAM,
  NODE_CALL,
  NODE_RETURN
} NodeType;

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
      char *name;  /* nombre de la funcion */
      struct ASTNode *params; /* lista de parametros */
      DataType returnType;      /* Si no devuelve nada */
      struct ASTNode *body;     /* sentencias del cuerpo */
    } func;
    struct {
     char *name;            /* función que se llama */
      struct ASTNode *args;  /* argumentos (enlazados con next) */
    } call;
    struct {
      struct ASTNode *value; /* NULL en "retornar;" */
    } ret;
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
ASTNode *create_func_node(const char *name, ASTNode *params, DataType ret_type, ASTNode *body, int line);
ASTNode *create_param_node(const char *name, DataType type, int line);
ASTNode *create_call_node(const char *name, ASTNode *args, int line);
ASTNode *create_return_node(ASTNode *value, int line);
int ast_list_length(const ASTNode *list);

DataType parse_type_name(const char *name);
const char *type_name(DataType type);
const char *operator_symbol(Operator op);

void print_ast_tree(const ASTNode *root);  /* arbol con indentacion */
void print_ast_table(const ASTNode *root); /* tabla de nodos        */
void free_ast(ASTNode *node);

#endif
