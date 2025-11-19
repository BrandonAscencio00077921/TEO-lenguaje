 #ifndef PYCLITE_AST_H
 #define PYCLITE_AST_H

#include "lexer/lexer.h"

 #include <stddef.h>

 typedef enum {
     AST_PROGRAM,
     AST_INSTRUCTION_LIST,
     AST_DECLARATION,
     AST_ASSIGNMENT,
     AST_IF,
     AST_FOR,
     AST_WHILE,
     AST_FUNCTION,
     AST_CALL,
     AST_RETURN,
     AST_EXPRESSION,
     AST_LITERAL,
     AST_IDENTIFIER,
     AST_ARRAY_LITERAL,
     AST_PARAM_LIST,
     AST_ARG_LIST,
     AST_COMMENT
 } ASTNodeType;

 typedef struct ASTNode {
     ASTNodeType type;
     Token token;
     struct ASTNode **children;
     size_t child_count;
 } ASTNode;

 ASTNode *ast_create(ASTNodeType type, Token token);
 void ast_add_child(ASTNode *parent, ASTNode *child);
 void ast_free(ASTNode *node);

 #endif // PYCLITE_AST_H

