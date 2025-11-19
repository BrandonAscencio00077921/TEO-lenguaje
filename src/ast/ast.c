 #include "ast.h"

 #include <stdlib.h>

 ASTNode *ast_create(ASTNodeType type, Token token) {
     ASTNode *node = (ASTNode *)calloc(1, sizeof(ASTNode));
     if (!node) {
         return NULL;
     }
     node->type = type;
     node->token = token;
     return node;
 }

 void ast_add_child(ASTNode *parent, ASTNode *child) {
     if (!parent || !child) {
         return;
     }
     size_t new_count = parent->child_count + 1;
     ASTNode **new_children = (ASTNode **)realloc(parent->children, new_count * sizeof(ASTNode *));
     if (!new_children) {
         return;
     }
     parent->children = new_children;
     parent->children[parent->child_count] = child;
     parent->child_count = new_count;
 }

 void ast_free(ASTNode *node) {
     if (!node) {
         return;
     }
     for (size_t i = 0; i < node->child_count; ++i) {
         ast_free(node->children[i]);
     }
     free(node->children);
     free(node);
 }

