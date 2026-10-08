#include "ast.h"
#include "lexer.h"
#include "runtime.h"
#include "semantic.h"
#include "symbol_table.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern FILE *yyin;        /* archivo de entrada de Flex */
extern int yyparse(void); /* punto de entrada de Bison  */
extern ASTNode *ast_root; /* raiz del AST (parser.y)    */
extern int syntax_errors;

static void print_usage(const char *program) {
  fprintf(stderr, "Uso: %s [-d] archivo.dap\n", program);
  fprintf(stderr, "  -d   modo detalle: muestra el AST, la tabla del AST y la tabla de simbolos\n");
}

int main(int argc, char **argv) {
  int debug = 0;
  const char *path = NULL;

  for (int i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-d") == 0) {
      debug = 1;
    } else if (argv[i][0] == '-' || path != NULL) {
      print_usage(argv[0]);
      return EXIT_FAILURE;
    } else {
      path = argv[i];
    }
  }

  if (path == NULL) {
    print_usage(argv[0]);
    return EXIT_FAILURE;
  }

  yyin = fopen(path, "r");
  if (yyin == NULL) {
    perror("No se pudo abrir el archivo");
    return EXIT_FAILURE;
  }

  SymbolTable table;
  symbol_table_init(&table);
  int status = EXIT_SUCCESS;

  /* Fase 1 y 2: analisis lexico + sintactico (Flex + Bison) -> AST */
  yyparse();
  fclose(yyin);

  if (lexical_errors > 0 || syntax_errors > 0) {
    fprintf(stderr, "\nCompilacion detenida: %d error(es) lexico(s), %d sintactico(s).\n",
            lexical_errors, syntax_errors);
    free_ast(ast_root);
    symbol_table_destroy(&table);
    return EXIT_FAILURE;
  }

  /* Fase 3: analisis semantico -> tabla de simbolos + tipos en el AST */
  int semantic_errors = semantic_analyze(ast_root, &table);

  if (debug) {
    printf("=== AST (arbol) ===\n");
    print_ast_tree(ast_root);
    printf("\n=== AST Table ===\n");
    print_ast_table(ast_root);
    printf("\n");
  }

  if (semantic_errors > 0) {
    fprintf(stderr, "\nCompilacion detenida: %d error(es) semantico(s).\n",
            semantic_errors);
    if (debug) {
      printf("=== Tabla de simbolos ===\n");
      symbol_table_print(&table);
    }
    free_ast(ast_root);
    symbol_table_destroy(&table);
    return EXIT_FAILURE;
  }

  /* Fase 4: ejecucion del programa */
  if (debug) {
    printf("=== Ejecucion ===\n");
  }
  if (!execute_program(ast_root, &table)) {
    status = EXIT_FAILURE;
  }

  if (debug) {
    printf("\n=== Tabla de simbolos ===\n");
    symbol_table_print(&table);
  }

  free_ast(ast_root);
  symbol_table_destroy(&table);
  return status;
}
