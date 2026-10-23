#include "ast.h"
#include "ast_internal.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Impresion del arbol y de la tabla del AST (modo -d) */

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
  case NODE_FUNC_DECL: return "Declaracion de funcion";
  case NODE_PARAM: return "Parametro";
  case NODE_CALL: return "Llamada a funcion";
  case NODE_RETURN: return "Retornar";
  case NODE_IF: return "Si (condicional)";
  case NODE_BLOCK: return "Bloque";
  case NODE_ARRAY: return "Literal arreglo";
  case NODE_INDEX: return "Acceso a arreglo";
  case NODE_ARRAY_CALL: return "Metodo de arreglo";
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
  case NODE_PARAM:
    snprintf(out, size, "%s", node->data.decl.name);
    break;
  case NODE_ASSIGN:
    snprintf(out, size, "%s", node->data.assign.name);
    break;
  case NODE_FUNC_DECL:
    snprintf(out, size, "%s", node->data.func.name);
    break;
  case NODE_CALL:
    snprintf(out, size, "%s", node->data.call.name);
    break;
  case NODE_PRINT:
  case NODE_RETURN:
  case NODE_IF:
    break;
  case NODE_BLOCK:
    snprintf(out, size, "%s", node->data.block.is_else ? "sino" : "si");
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
  case NODE_INDEX:
    snprintf(out, size, "%s", node->data.index.name);
    break;
  case NODE_ARRAY_CALL:
    snprintf(out, size, "%s.%s", node->data.array_call.name,
             node->data.array_call.method_name);
    break;
  case NODE_ARRAY:
    break;
  case NODE_BINARY:
  case NODE_UNARY:
    snprintf(out, size, "%s", operator_symbol(node->data.binary.op));
    break;
  }
}

/* El nodo numero "index" de una lista enlazada con next (o NULL) */
static const ASTNode *list_nth(const ASTNode *list, int index) {
  while (list != NULL && index > 0) {
    list = list->next;
    index--;
  }
  return list;
}

/* El hijo numero "index" de un nodo (o NULL si no hay mas hijos) */
static const ASTNode *node_child(const ASTNode *node, int index) {
  switch (node->type) {
  case NODE_VAR_DECL:
  case NODE_CONST_DECL:
    return index == 0 ? node->data.decl.init : NULL;
  case NODE_ASSIGN:
    return index == 0 ? node->data.assign.value : NULL;
  case NODE_PRINT:
    return index == 0 ? node->data.print.value : NULL;
  case NODE_RETURN:
    return index == 0 ? node->data.ret.value : NULL;
  case NODE_BINARY:
    if (index == 0) return node->data.binary.left;
    if (index == 1) return node->data.binary.right;
    return NULL;
  case NODE_UNARY:
    return index == 0 ? node->data.binary.left : NULL;
  case NODE_CALL:
    return list_nth(node->data.call.args, index);
  case NODE_IF:
    if (index == 0) return node->data.if_stmt.cond;
    if (index == 1) return node->data.if_stmt.then_block;
    if (index == 2) return node->data.if_stmt.else_block;
    return NULL;
  case NODE_BLOCK:
    return list_nth(node->data.block.body, index);
  case NODE_ARRAY:
    return list_nth(node->data.array.elements, index);
  case NODE_INDEX:
    return index == 0 ? node->data.index.index : NULL;
  case NODE_ARRAY_CALL:
    return list_nth(node->data.array_call.args, index);
  case NODE_FUNC_DECL: {
    /* primero los parametros, despues las sentencias del cuerpo */
    int params = ast_list_length(node->data.func.params);
    if (index < params) {
      return list_nth(node->data.func.params, index);
    }
    return list_nth(node->data.func.body, index - params);
  }
  default:
    return NULL;
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

  for (int i = 0; node_child(node, i) != NULL; i++) {
    print_tree_node(node_child(node, i), next_prefix,
                    node_child(node, i + 1) == NULL);
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
      ast_out_of_memory();
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
  for (int i = 0; node_child(node, i) != NULL; i++) {
    add_row(list, node_child(node, i), my_id);
  }
}

void print_ast_table(const ASTNode *root) {
  AstRows list = {NULL, 0, 0};

  /* Fila 1: la raiz "Programa" */
  list.rows = malloc(32 * sizeof(AstRow));
  if (list.rows == NULL) {
    ast_out_of_memory();
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