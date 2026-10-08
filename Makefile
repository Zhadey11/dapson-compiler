CC        := gcc
FLEX      := flex
BISON     := bison

BUILD_DIR := build
CFLAGS    := -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -I. -I$(BUILD_DIR)
LDFLAGS   :=

TARGET    := dapson
HEADERS   := ast.h symbol_table.h semantic.h runtime.h string_utils.h lexer.h
LEX_SRC   := lexer.l
YACC_SRC  := parser.y

# Archivos generados por Bison y Flex
BISON_C   := $(BUILD_DIR)/parser.tab.c
BISON_H   := $(BUILD_DIR)/parser.tab.h
LEX_C     := $(BUILD_DIR)/lex.yy.c

OBJS      := $(BUILD_DIR)/main.o \
             $(BUILD_DIR)/ast.o \
             $(BUILD_DIR)/symbol_table.o \
             $(BUILD_DIR)/semantic.o \
             $(BUILD_DIR)/runtime.o \
             $(BUILD_DIR)/string_utils.o \
             $(BUILD_DIR)/lexer_support.o \
             $(BUILD_DIR)/parser.tab.o \
             $(BUILD_DIR)/lex.yy.o

.PHONY: all clean run debug errores

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Archivos C normales
$(BUILD_DIR)/%.o: %.c $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Bison: parser.y -> parser.tab.c / parser.tab.h
$(BISON_C) $(BISON_H): $(YACC_SRC) | $(BUILD_DIR)
	$(BISON) -d -o $(BISON_C) $(YACC_SRC)

$(BUILD_DIR)/parser.tab.o: $(BISON_C) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Wno-unused-function -c $(BISON_C) -o $@

# Flex: lexer.l -> lex.yy.c (necesita los tokens que genera Bison)
$(LEX_C): $(LEX_SRC) $(BISON_H) | $(BUILD_DIR)
	$(FLEX) -o $(LEX_C) $(LEX_SRC)

$(BUILD_DIR)/lex.yy.o: $(LEX_C) $(HEADERS) | $(BUILD_DIR)
	$(CC) $(CFLAGS) -Wno-unused-function -Wno-sign-compare -c $(LEX_C) -o $@

$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# make run     -> ejecuta el programa de ejemplo
run: $(TARGET)
	./$(TARGET) programa.dap

# make debug   -> igual, mostrando AST, tabla del AST y tabla de simbolos
debug: $(TARGET)
	./$(TARGET) -d programa.dap

# make errores -> programa con errores semanticos a proposito
errores: $(TARGET)
	./$(TARGET) errores.dap

clean:
	rm -rf $(BUILD_DIR) $(TARGET)
