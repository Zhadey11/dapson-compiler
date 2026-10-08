#include "ast.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ------------------------------------------------------------------ */
/* Construccion de nodos                                               */
/* ------------------------------------------------------------------ */

static void out_of_memory(void) {
  fprintf(stderr, "Sin memoria\n");
  exit(EXIT_FAILURE);
}

static ASTNode *new_node(NodeType type, int line) {
  ASTNode *node = calloc(1, sizeof(*node));
  if (node == NULL) {
    out_of_memory();
  }
  node->type = type;
  node->exprType = TYPE_UNKNOWN;
  node->line = line;
  return node;
}

static char *copy_or_die(const char *value) {
  char *copy = string_copy(value);
  if (copy == NULL) {
    out_of_memory();
  }
  return copy;
}

ASTNode *create_int_node(long long val, int line) {
  ASTNode *node = new_node(NODE_INT, line);
  node->data.intValue = val;
  return node;
}

ASTNode *create_decimal_node(double val, int line) {
  ASTNode *node = new_node(NODE_DECIMAL, line);
  node->data.decimalValue = val;
  return node;
}

ASTNode *create_text_node(const char *val, int line) {
  ASTNode *node = new_node(NODE_TEXT, line);
  node->data.strValue = copy_or_die(val);
  return node;
}

ASTNode *create_char_node(char val, int line) {
  ASTNode *node = new_node(NODE_CHAR, line);
  node->data.charValue = val;
  return node;
}

ASTNode *create_bool_node(int val, int line) {
  ASTNode *node = new_node(NODE_BOOL, line);
  node->data.boolValue = val;
  return node;
}

ASTNode *create_identifier_node(const char *name, int line) {
  ASTNode *node = new_node(NODE_IDENT, line);
  node->data.identifier = copy_or_die(name);
  return node;
}

ASTNode *create_binary_node(Operator op, ASTNode *left, ASTNode *right,
                            int line) {
  ASTNode *node = new_node(NODE_BINARY, line);
  node->data.binary.op = op;
  node->data.binary.left = left;
  node->data.binary.right = right;
  return node;
}

ASTNode *create_unary_node(Operator op, ASTNode *operand, int line) {
  ASTNode *node = new_node(NODE_UNARY, line);
  node->data.binary.op = op;
  node->data.binary.left = operand;
  node->data.binary.right = NULL;
  return node;
}

ASTNode *create_decl_node(NodeType kind, const char *name, DataType type,
                          ASTNode *init, int line) {
  ASTNode *node = new_node(kind, line);
  node->exprType = type; /* el tipo declarado */
  node->data.decl.name = copy_or_die(name);
  node->data.decl.init = init;
  return node;
}

ASTNode *create_assign_node(const char *name, ASTNode *value, int line) {
  ASTNode *node = new_node(NODE_ASSIGN, line);
  node->data.assign.name = copy_or_die(name);
  node->data.assign.value = value;
  return node;
}

ASTNode *create_print_node(ASTNode *value, int line) {
  ASTNode *node = new_node(NODE_PRINT, line);
  node->data.print.value = value;
  return node;
}

/* ------------------------------------------------------------------ */
/* Nombres                                                             */
/* ------------------------------------------------------------------ */

DataType parse_type_name(const char *name) {
  if (strcmp(name, "entero") == 0) return TYPE_ENTERO;
  if (strcmp(name, "decimal") == 0) return TYPE_DECIMAL;
  if (strcmp(name, "texto") == 0) return TYPE_TEXTO;
  if (strcmp(name, "caracter") == 0) return TYPE_CARACTER;
  if (strcmp(name, "booleano") == 0) return TYPE_BOOLEANO;
  return TYPE_UNKNOWN;
}

const char *type_name(DataType type) {
  switch (type) {
  case TYPE_ENTERO: return "entero";
  case TYPE_DECIMAL: return "decimal";
  case TYPE_TEXTO: return "texto";
  case TYPE_CARACTER: return "caracter";
  case TYPE_BOOLEANO: return "booleano";
  case TYPE_UNKNOWN: break;
  }
  return "desconocido";
}

const char *operator_symbol(Operator op) {
  switch (op) {
  case OP_ADD: return "+";
  case OP_SUB: return "-";
  case OP_MUL: return "*";
  case OP_DIV: return "/";
  case OP_NEG: return "-";
  }
  return "?";
}

static const char *node_kind(const ASTNode *node) {
  switch (node->type) {
  case NODE_VAR_DECL: return "Declaracion de variable";
  case NODE_CONST_DECL: return "Declaracion de constante";
  case NODE_ASSIGN: return "Asignacion";
  case NODE_PRINT: return "Imprimir";
  case NODE_INT: return "Literal entero";
  case NODE_DECIMAL: return "Literal decimal";
  case NODE_TEXT: return "Literal texto";
  case NODE_CHAR: return "Literal caracter";
  case NODE_BOOL: return "Literal booleano";
  case NODE_IDENT: return "Identificador";
  case NODE_BINARY: return "Operacion binaria";
  case NODE_UNARY: return "Operacion unaria";
  }
  return "?";
}

/* Copia "text" a "out" escapando saltos de linea, tabs, comillas, etc. */
static void append_escaped(char *out, size_t size, const char *text) {
  size_t used = strlen(out);
  for (; *text != '\0' && used + 3 < size; text++) {
    switch (*text) {
    case '\n': out[used++] = '\\'; out[used++] = 'n'; break;
    case '\t': out[used++] = '\\'; out[used++] = 't'; break;
    case '\r': out[used++] = '\\'; out[used++] = 'r'; break;
    case '\\': out[used++] = '\\'; out[used++] = '\\'; break;
    default: out[used++] = *text; break;
    }
  }
  out[used] = '\0';
}

static void node_detail(const ASTNode *node, char *out, size_t size) {
  char tmp[2] = {0, 0};
  out[0] = '\0';
  switch (node->type) {
  case NODE_VAR_DECL:
  case NODE_CONST_DECL:
    snprintf(out, size, "%s", node->data.decl.name);
    break;
  case NODE_ASSIGN:
    snprintf(out, size, "%s", node->data.assign.name);
    break;
  case NODE_PRINT:
    break;
  case NODE_INT:
    snprintf(out, size, "%lld", node->data.intValue);
    break;
  case NODE_DECIMAL:
    snprintf(out, size, "%g", node->data.decimalValue);
    break;
  case NODE_TEXT:
    snprintf(out, size, "\"");
    append_escaped(out, size, node->data.strValue);
    strncat(out, "\"", size - strlen(out) - 1);
    break;
  case NODE_CHAR:
    snprintf(out, size, "'");
    tmp[0] = node->data.charValue;
    append_escaped(out, size, tmp);
    strncat(out, "'", size - strlen(out) - 1);
    break;
  case NODE_BOOL:
    snprintf(out, size, "%s", node->data.boolValue ? "verdadero" : "falso");
    break;
  case NODE_IDENT:
    snprintf(out, size, "%s", node->data.identifier);
    break;
  case NODE_BINARY:
  case NODE_UNARY:
    snprintf(out, size, "%s", operator_symbol(node->data.binary.op));
    break;
  }
}

static int node_children(const ASTNode *node, const ASTNode *kids[2]) {
  switch (node->type) {
  case NODE_VAR_DECL:
  case NODE_CONST_DECL:
    kids[0] = node->data.decl.init;
    return kids[0] != NULL ? 1 : 0;
  case NODE_ASSIGN:
    kids[0] = node->data.assign.value;
    return 1;
  case NODE_PRINT:
    kids[0] = node->data.print.value;
    return 1;
  case NODE_BINARY:
    kids[0] = node->data.binary.left;
    kids[1] = node->data.binary.right;
    return 2;
  case NODE_UNARY:
    kids[0] = node->data.binary.left;
    return 1;
  default:
    return 0;
  }
}

/* ------------------------------------------------------------------ */
/* Impresion del arbol                                                 */
/* ------------------------------------------------------------------ */

static void print_tree_node(const ASTNode *node, const char *prefix,
                            int is_last) {
  char detail[160];
  node_detail(node, detail, sizeof(detail));

  printf("%s%s%s", prefix, is_last ? "`-- " : "|-- ", node_kind(node));
  if (detail[0] != '\0') {
    printf(": %s", detail);
  }
  if (node->exprType != TYPE_UNKNOWN) {
    printf("  <%s>", type_name(node->exprType));
  }
  putchar('\n');

  char next_prefix[256];
  snprintf(next_prefix, sizeof(next_prefix), "%s%s", prefix,
           is_last ? "    " : "|   ");

  const ASTNode *kids[2] = {NULL, NULL};
  int count = node_children(node, kids);
  for (int i = 0; i < count; i++) {
    print_tree_node(kids[i], next_prefix, i == count - 1);
  }
}

void print_ast_tree(const ASTNode *root) {
  puts("Programa");
  for (const ASTNode *node = root; node != NULL; node = node->next) {
    print_tree_node(node, "", node->next == NULL);
  }
}

/* ------------------------------------------------------------------ */
/* Impresion de la tabla del AST                                       */
/* ------------------------------------------------------------------ */

typedef struct {
  int id;
  int parent;
  int line;
  const char *kind;
  char detail[160];
  const char *type;
} AstRow;

typedef struct {
  AstRow *rows;
  int count;
  int capacity;
} AstRows;

static void add_row(AstRows *list, const ASTNode *node, int parent) {
  if (list->count == list->capacity) {
    list->capacity = list->capacity == 0 ? 32 : list->capacity * 2;
    list->rows = realloc(list->rows, (size_t)list->capacity * sizeof(AstRow));
    if (list->rows == NULL) {
      out_of_memory();
    }
  }

  AstRow *row = &list->rows[list->count];
  row->id = list->count + 1;
  row->parent = parent;
  row->line = node->line;
  row->kind = node_kind(node);
  node_detail(node, row->detail, sizeof(row->detail));
  row->type = node->exprType != TYPE_UNKNOWN ? type_name(node->exprType) : "-";
  list->count++;

  int my_id = row->id; /* "row" puede invalidarse con el realloc de abajo */
  const ASTNode *kids[2] = {NULL, NULL};
  int kid_count = node_children(node, kids);
  for (int i = 0; i < kid_count; i++) {
    add_row(list, kids[i], my_id);
  }
}

void print_ast_table(const ASTNode *root) {
  AstRows list = {NULL, 0, 0};

  /* Fila 1: la raiz "Programa" */
  list.rows = malloc(32 * sizeof(AstRow));
  if (list.rows == NULL) {
    out_of_memory();
  }
  list.capacity = 32;
  list.rows[0] = (AstRow){1, 0, 0, "Programa", "", "-"};
  list.count = 1;

  for (const ASTNode *node = root; node != NULL; node = node->next) {
    add_row(&list, node, 1);
  }

  const char *headers[6] = {"ID", "Padre", "Nodo", "Detalle", "Tipo", "Linea"};
  size_t widths[6];
  for (int i = 0; i < 6; i++) {
    widths[i] = utf8_length(headers[i]);
  }

  char number[32];
  for (int i = 0; i < list.count; i++) {
    AstRow *row = &list.rows[i];
    snprintf(number, sizeof(number), "%d", row->id);
    if (utf8_length(number) > widths[0]) widths[0] = utf8_length(number);
    snprintf(number, sizeof(number), "%d", row->parent);
    if (utf8_length(number) > widths[1]) widths[1] = utf8_length(number);
    if (utf8_length(row->kind) > widths[2]) widths[2] = utf8_length(row->kind);
    if (utf8_length(row->detail) > widths[3]) widths[3] = utf8_length(row->detail);
    if (utf8_length(row->type) > widths[4]) widths[4] = utf8_length(row->type);
    snprintf(number, sizeof(number), "%d", row->line);
    if (utf8_length(number) > widths[5]) widths[5] = utf8_length(number);
  }

  table_print_rule(widths, 6);
  for (int i = 0; i < 6; i++) {
    table_print_cell(headers[i], widths[i]);
  }
  puts("|");
  table_print_rule(widths, 6);

  for (int i = 0; i < list.count; i++) {
    AstRow *row = &list.rows[i];
    snprintf(number, sizeof(number), "%d", row->id);
    table_print_cell(number, widths[0]);
    if (row->parent == 0) {
      table_print_cell("-", widths[1]);
    } else {
      snprintf(number, sizeof(number), "%d", row->parent);
      table_print_cell(number, widths[1]);
    }
    table_print_cell(row->kind, widths[2]);
    table_print_cell(row->detail, widths[3]);
    table_print_cell(row->type, widths[4]);
    if (row->line == 0) {
      table_print_cell("-", widths[5]);
    } else {
      snprintf(number, sizeof(number), "%d", row->line);
      table_print_cell(number, widths[5]);
    }
    puts("|");
  }
  table_print_rule(widths, 6);

  free(list.rows);
}

/* ------------------------------------------------------------------ */
/* Liberacion de memoria                                               */
/* ------------------------------------------------------------------ */

void free_ast(ASTNode *node) {
  while (node != NULL) {
    ASTNode *next_node = node->next;

    switch (node->type) {
    case NODE_VAR_DECL:
    case NODE_CONST_DECL:
      free(node->data.decl.name);
      free_ast(node->data.decl.init);
      break;
    case NODE_ASSIGN:
      free(node->data.assign.name);
      free_ast(node->data.assign.value);
      break;
    case NODE_PRINT:
      free_ast(node->data.print.value);
      break;
    case NODE_TEXT:
      free(node->data.strValue);
      break;
    case NODE_IDENT:
      free(node->data.identifier);
      break;
    case NODE_BINARY:
    case NODE_UNARY:
      free_ast(node->data.binary.left);
      free_ast(node->data.binary.right);
      break;
    case NODE_INT:
    case NODE_DECIMAL:
    case NODE_CHAR:
    case NODE_BOOL:
      break;
    }

    free(node);
    node = next_node;
  }
}
