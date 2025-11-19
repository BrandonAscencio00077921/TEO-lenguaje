CC = gcc
CFLAGS = -std=c17 -Wall -Wextra -pedantic -g -Isrc -Isrc/lexer -Isrc/parser -Isrc/ast

SRC = \
	src/main.c \
	src/lexer/lexer.c \
	src/parser/parser.c \
	src/ast/ast.c
 OBJ = $(SRC:.c=.o)

 TARGET = pyclitec

 all: $(TARGET)

 $(TARGET): $(OBJ)
	$(CC) $(CFLAGS) -o $@ $(OBJ)

 %.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

 clean:
	rm -f $(OBJ) $(TARGET)

 .PHONY: all clean

