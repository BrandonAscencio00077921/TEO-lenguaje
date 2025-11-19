#include "parser/parser.h"

 #include <stdio.h>
 #include <stdlib.h>

 static char *read_file(const char *path, size_t *out_size) {
     FILE *file = fopen(path, "rb");
     if (!file) {
         return NULL;
     }
     fseek(file, 0, SEEK_END);
     long size = ftell(file);
     fseek(file, 0, SEEK_SET);

     if (size < 0) {
         fclose(file);
         return NULL;
     }

     char *buffer = (char *)malloc((size_t)size + 1);
     if (!buffer) {
         fclose(file);
         return NULL;
     }

     size_t read = fread(buffer, 1, (size_t)size, file);
     fclose(file);
     buffer[read] = '\0';
     if (out_size) {
         *out_size = read;
     }
     return buffer;
 }

 int main(int argc, char **argv) {
     if (argc < 2) {
         fprintf(stderr, "Uso: %s <archivo.pycl>\n", argv[0]);
         return 1;
     }

     size_t source_size = 0;
     char *source = read_file(argv[1], &source_size);
     if (!source) {
         fprintf(stderr, "No se pudo leer el archivo: %s\n", argv[1]);
         return 1;
     }

     Parser parser;
     parser_init(&parser, source, source_size);
     ASTNode *program = parser_parse(&parser);

     if (parser_has_error(&parser) || !program) {
         Token error_token = parser_error_token(&parser);
         fprintf(stderr, "Error de parseo en línea %zu, columna %zu: %s\n",
                 error_token.line, error_token.column, parser_error_message(&parser));
         free(source);
         return 1;
     }

     printf("Parseo completado correctamente.\n");
     ast_free(program);
     free(source);
     return 0;
 }

