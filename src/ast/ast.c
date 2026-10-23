#include "ast.h"
#include "ast_internal.h"
#include "string_utils.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Construccion de los nodos del arbol (AST) */

void ast_out_of_memory(void) {
  fprintf(stderr, "Sin memoria\n");
  exit(EXIT_FAILURE);
}

static ASTNode *new_node(NodeType type, int line) {
  ASTNode *node = calloc(1, sizeof(*node));
  if (node == NULL) {
    ast_out_of_memory();
  }
  node->type = type;
  node->exprType = TYPE_UNKNOWN;
  node->line = line;
  return node;
}

static char *copy_or_die(const char *value) {
  char *copy = string_copy(value);
  if (copy == NULL) {
    ast_out_of_memory();
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

ASTNode *create_func_node(const char *name, ASTNode *params, DataType ret_type,
                          ASTNode *body, int line) {
  ASTNode *node = new_node(NODE_FUNC_DECL, line);
  node->exprType = ret_type; /* el tipo que devuelve la funcion */
  node->data.func.name = copy_or_die(name);
  node->data.func.params = params;
  node->data.func.body = body;
  return node;
}

ASTNode *create_param_node(const char *name, DataType type, int line) {
  ASTNode *node = new_node(NODE_PARAM, line);
  node->exprType = type;
  node->data.decl.name = copy_or_die(name);
  node->data.decl.init = NULL;
  return node;
}

ASTNode *create_call_node(const char *name, ASTNode *args, int line) {
  ASTNode *node = new_node(NODE_CALL, line);
  node->data.call.name = copy_or_die(name);
  node->data.call.args = args;
  return node;
}

ASTNode *create_return_node(ASTNode *value, int line) {
  ASTNode *node = new_node(NODE_RETURN, line);
  node->data.ret.value = value;
  return node;
}

ASTNode *create_block_node(ASTNode *body, int line) {
  ASTNode *node = new_node(NODE_BLOCK, line);
  node->data.block.body = body;
  node->data.block.is_else = 0;
  return node;
}

ASTNode *create_if_node(ASTNode *cond, ASTNode *then_block,
                        ASTNode *else_block, int line) {
  ASTNode *node = new_node(NODE_IF, line);
  node->data.if_stmt.cond = cond;
  node->data.if_stmt.then_block = then_block;
  node->data.if_stmt.else_block = else_block;
  return node;
}

ASTNode *create_array_node(ASTNode *elements, int line) {
  ASTNode *node = new_node(NODE_ARRAY, line);
  node->data.array.elements = elements;
  return node;
}

ASTNode *create_index_node(const char *name, ASTNode *index, int line) {
  ASTNode *node = new_node(NODE_INDEX, line);
  node->data.index.name = copy_or_die(name);
  node->data.index.index = index;
  return node;
}

/* Convierte el nombre escrito (agregar, quitar, largo) en un ArrayMethod */
static ArrayMethod array_method_from_name(const char *name) {
  if (strcmp(name, "agregar") == 0) return ARRAY_AGREGAR;
  if (strcmp(name, "quitar") == 0) return ARRAY_QUITAR;
  if (strcmp(name, "largo") == 0) return ARRAY_LARGO;
  return ARRAY_DESCONOCIDO;
}

ASTNode *create_array_call_node(const char *name, const char *method_name,
                                ASTNode *args, int line) {
  ASTNode *node = new_node(NODE_ARRAY_CALL, line);
  node->data.array_call.name = copy_or_die(name);
  node->data.array_call.method_name = copy_or_die(method_name);
  node->data.array_call.method = array_method_from_name(method_name);
  node->data.array_call.args = args;
  return node;
}

int ast_list_length(const ASTNode *list) {
  int count = 0;
  for (; list != NULL; list = list->next) {
    count++;
  }
  return count;
}