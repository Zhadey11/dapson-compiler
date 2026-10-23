CC        := gcc
FLEX      := flex
BISON     := bison

SRC_DIR   := src
INC_DIR   := include
EX_DIR    := examples
BUILD_DIR := build
CFLAGS    := -Wall -Wextra -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -I$(INC_DIR) -I$(BUILD_DIR)
LDFLAGS   :=

TARGET    := dapson

# Todos los .c de src/ y de sus carpetas (ast, lexer, parser, semantic, runtime)
SOURCES   := $(wildcard $(SRC_DIR)/*.c \
                        $(SRC_DIR)/ast/*.c \
                        $(SRC_DIR)/lexer/*.c \
                        $(SRC_DIR)/parser/*.c \
                        $(SRC_DIR)/semantic/*.c \
                        $(SRC_DIR)/runtime/*.c)
HEADERS   := $(wildcard $(INC_DIR)/*.h $(SRC_DIR)/*/*.h)
OBJS      := $(patsubst $(SRC_DIR)/%.c,$(BUILD_DIR)/%.o,$(SOURCES))

LEX_SRC   := $(SRC_DIR)/lexer/lexer.l
YACC_SRC  := $(SRC_DIR)/parser/parser.y

# Archivos generados por Bison y Flex
BISON_C   := $(BUILD_DIR)/parser.tab.c
BISON_H   := $(BUILD_DIR)/parser.tab.h
LEX_C     := $(BUILD_DIR)/lex.yy.c

OBJS      += $(BUILD_DIR)/parser.tab.o $(BUILD_DIR)/lex.yy.o

.PHONY: all clean run debug errores funciones errores-funciones condicionales errores-si arreglos errores-arreglos

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(OBJS) $(LDFLAGS) -o $@

# Archivos C normales (src/*.c y src/*/*.c)
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c $(HEADERS) | $(BUILD_DIR)
	@mkdir -p $(dir $@)
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
	./$(TARGET) $(EX_DIR)/programa.dap

# make debug   -> igual, mostrando AST, tabla del AST y tabla de simbolos
debug: $(TARGET)
	./$(TARGET) -d $(EX_DIR)/programa.dap

# make errores -> programa con errores semanticos a proposito
errores: $(TARGET)
	./$(TARGET) $(EX_DIR)/errores.dap

# make funciones -> programa de ejemplo con funciones
funciones: $(TARGET)
	./$(TARGET) $(EX_DIR)/funciones.dap

# make errores-funciones -> errores semanticos de funciones a proposito
errores-funciones: $(TARGET)
	./$(TARGET) $(EX_DIR)/errores_funciones.dap

# make condicionales -> programa de ejemplo con si / sino
condicionales: $(TARGET)
	./$(TARGET) $(EX_DIR)/condicionales.dap

# make errores-si -> errores semanticos de condicionales a proposito
errores-si: $(TARGET)
	./$(TARGET) $(EX_DIR)/errores_si.dap

# make arreglos -> programa de ejemplo con arreglos
arreglos: $(TARGET)
	./$(TARGET) $(EX_DIR)/arreglos.dap

# make errores-arreglos -> errores semanticos de arreglos a proposito
errores-arreglos: $(TARGET)
	./$(TARGET) $(EX_DIR)/errores_arreglos.dap

clean:
	rm -rf $(BUILD_DIR) $(TARGET)