#include "runtime_internal.h"

#include <stdlib.h>

/* Maximo de llamadas anidadas (evita desbordar la pila con recursion infinita) */
#define MAX_CALL_DEPTH 500

/* Llama a una funcion: evalua los argumentos, crea un marco nuevo y ejecuta el cuerpo */
int runtime_call_function(const ASTNode *node, Frame *frame, Value *out) {
  const char *name = node->data.call.name;
  Function *function = function_lookup(frame->global, name);
  if (function == NULL) {
    return runtime_error(node->line, "la funcion '%s' no existe", name);
  }
  if (frame->depth >= MAX_CALL_DEPTH) {
    return runtime_error(node->line, "demasiadas llamadas anidadas en '%s'", name);
  }

  /* 1. los argumentos se evaluan en el marco de quien llama */
  Value *args = calloc((size_t)function->param_count + 1, sizeof(Value));
  if (args == NULL) {
    return runtime_error(node->line, "sin memoria al llamar a '%s'", name);
  }
  int index = 0;
  for (const ASTNode *arg = node->data.call.args; arg != NULL; arg = arg->next) {
    if (!runtime_evaluate(arg, frame, &args[index])) {
      free(args);
      return 0;
    }
    index++;
  }

  /* 2. marco nuevo: los parametros son sus primeras variables */
  SymbolTable locals;
  symbol_table_init(&locals);
  Frame callee = {frame->global, &locals, frame->depth + 1, 0, {TYPE_NADA, {0}}};
  int ok = 1;
  for (int i = 0; i < function->param_count && ok; i++) {
    if (symbol_table_insert(&locals, function->param_names[i],
                            function->param_types[i], 0, function->line) <= 0) {
      ok = runtime_error(node->line, "sin memoria al llamar a '%s'", name);
    } else {
      Symbol *param = symbol_table_lookup(&locals, function->param_names[i]);
      param->is_param = 1;
      runtime_store(param, &args[i]);
    }
  }
  free(args);

  /* 3. se ejecuta el cuerpo de la funcion */
  if (ok) {
    ok = runtime_execute_list(function->node->data.func.body, &callee);
  }

  /* 4. el resultado: lo que dijo 'retornar' (un entero se ensancha a decimal) */
  if (ok) {
    if (function->return_type == TYPE_NADA) {
      out->type = TYPE_NADA;
    } else if (!callee.returned) {
      ok = runtime_error(node->line, "la funcion '%s' termino sin retornar un valor", name);
    } else {
      *out = callee.ret;
      runtime_widen(out, function->return_type);
    }
  }

  symbol_table_destroy(&locals);
  return ok;
}