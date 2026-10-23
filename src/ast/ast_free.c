#include "ast.h"

#include <stdlib.h>

/* Liberacion de memoria del arbol */

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
    case NODE_FUNC_DECL:
      free(node->data.func.name);
      free_ast(node->data.func.params);
      free_ast(node->data.func.body);
      break;
    case NODE_PARAM:
      free(node->data.decl.name);
      break;
    case NODE_CALL:
      free(node->data.call.name);
      free_ast(node->data.call.args);
      break;
    case NODE_RETURN:
      free_ast(node->data.ret.value);
      break;
    case NODE_IF:
      free_ast(node->data.if_stmt.cond);
      free_ast(node->data.if_stmt.then_block);
      free_ast(node->data.if_stmt.else_block);
      break;
    case NODE_BLOCK:
      free_ast(node->data.block.body);
      break;
    case NODE_ARRAY:
      free_ast(node->data.array.elements);
      break;
    case NODE_INDEX:
      free(node->data.index.name);
      free_ast(node->data.index.index);
      break;
    case NODE_ARRAY_CALL:
      free(node->data.array_call.name);
      free(node->data.array_call.method_name);
      free_ast(node->data.array_call.args);
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
    case NODE_MOVE: /* NUEVO: solo guarda dos numeros, no hay nada que liberar */
      break;
    }

    free(node);
    node = next_node;
  }
}