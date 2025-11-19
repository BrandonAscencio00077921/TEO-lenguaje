# PyCLite Compiler Toolkit

Este repositorio contiene los primeros componentes de un compilador para el lenguaje PyCLite, basado en la especificación incluida en `PyCLite.pdf`. Actualmente se incluyen:

- **Analizador léxico** (`src/lexer.c`): tokeniza código fuente PyCLite, reconoce palabras reservadas, identificadores, literales, operadores y comentarios.
- **Analizador sintáctico** (`src/parser.c`): implementa un parser LL(1) recursivo que construye un AST a partir de las reglas descritas en la gramática.
- **Construcción del AST** (`src/ast.c`): utilidades para crear y liberar nodos del árbol sintáctico.
- **Binario de prueba** (`src/main.c`): lee un archivo PyCLite, ejecuta el lexer y el parser, e informa si el proceso finalizó sin errores.

## Requisitos

- Compilador C compatible con C17 (por ejemplo, GCC o Clang).
- En sistemas tipo Unix disponer de `make`. En Windows se puede usar MSYS2/MinGW, WSL o compilar manualmente con el comando indicado más abajo.

## Compilación

```bash
make
```

Si `make` no está disponible, compila ejecutando:

```bash
gcc -std=c17 -Wall -Wextra -pedantic -g -o pyclitec \
  src/main.c src/lexer.c src/parser.c src/ast.c
```

## Uso

```bash
./pyclitec ejemplo.pycl
```

El programa imprimirá `Parseo completado correctamente.` si no se detectaron errores sintácticos. En caso contrario mostrará la línea, columna y descripción del problema encontrado.

## Próximos pasos sugeridos

- Extender el AST con información semántica (tipos, tablas de símbolos, etc.).
- Implementar una etapa de generación de código o traducción a un lenguaje intermedio.
- Añadir una suite de pruebas automatizadas y ejemplos de código PyCLite.