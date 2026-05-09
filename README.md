# Proyecto 3 - Editor CEZ

Editor de texto en C orientado a conceptos de sistemas operativos. El proyecto implementa un editor simple en consola con una estructura de datos tipo **Gap Buffer**, lectura y escritura de archivos con llamadas de bajo nivel tipo POSIX, y un formato binario propio (`.cez`) pensado para almacenar texto comprimido junto con metadatos e integridad por `CRC32`.

## Integrantes

- Daniel Arcila
- Jeronimo Contreras
- Juan Esteban Peña

## Descripción general

El programa principal permite:

- Escribir texto desde consola.
- Borrar caracteres.
- Visualizar el contenido actual del buffer.
- Guardar el contenido en un archivo `.cez`.
- Abrir nuevamente un archivo `.cez`.
- Renombrar el archivo de trabajo.

La idea central del proyecto es integrar varios temas vistos en sistemas operativos y programación de sistemas:

- Manejo de memoria dinámica.
- Edición eficiente de texto con `Gap Buffer`.
- Entrada/salida con `open`, `read`, `write` y `close`.
- Uso de páginas de memoria lógicas de 4 KB para I/O.
- Persistencia en un formato binario propio.
- Validación de integridad mediante `CRC32`.
- Compresión usando `zlib`.

## Arquitectura del proyecto

### 1. Editor de texto

El módulo [`editor.c`](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/editor.c) implementa un **Gap Buffer**, una estructura eficiente para inserciones y borrados cerca del cursor.

Funciones principales:

- `gb_create()`: crea el buffer.
- `gb_destroy()`: libera memoria.
- `gb_insert()`: inserta un carácter.
- `gb_delete()`: elimina el carácter anterior al cursor.
- `gb_move_cursor()`: mueve el cursor.
- `gb_get_text()`: obtiene una copia lineal del contenido.

### 2. Entrada/salida de bajo nivel

El módulo [`io.c`](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/io.c) maneja archivos usando syscalls estilo POSIX:

- `open()`
- `read()`
- `write()`
- `close()`
- `fstat()`

La lectura y escritura se hace por bloques de `PAGE_SIZE = 4096` bytes.

### 3. Formato de archivo `.cez`

El encabezado del formato está definido en [`format.h`](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/format.h).

Cada archivo `.cez` contiene:

1. Un `FileHeader` de 64 bytes.
2. Un payload comprimido.

Metadatos incluidos:

- `magic`: identificador del formato.
- `version`: versión del archivo.
- `flags`: banderas de compresión o futuras extensiones.
- `original_size`: tamaño del texto sin comprimir.
- `compressed_size`: tamaño del payload comprimido.
- `crc32_checksum`: verificación de integridad.
- `timestamp`: marca de tiempo Unix.
- `filename`: nombre del archivo.

### 4. Programa principal

El archivo [`main.c`](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/main.c) orquesta el flujo del editor:

- Obtiene el texto del `Gap Buffer`.
- Lo comprime antes de guardarlo.
- Construye el encabezado `.cez`.
- Verifica `CRC32` al abrir archivos.
- Descomprime el contenido antes de volverlo a cargar al editor.

## Estructura del repositorio

```text
.
├── main.c
├── editor.c
├── editor.h
├── io.c
├── io.h
├── format.h
├── compress.c
├── compress.h
├── benchmark_clasico.c
├── benchmark_nuestro.c
└── Makefile
```

## Requisitos

Para compilar el proyecto se necesita lo siguiente:

- Compilador `gcc`
- `make` opcional
- Biblioteca `zlib`
- Un entorno compatible con llamadas POSIX

En Linux/WSL, por ejemplo:

```bash
sudo apt update
sudo apt install build-essential zlib1g-dev
```

## Compilación

### Opción 1. Con Makefile

```bash
make
```

Esto debería generar:

- `editor`
- `benchmark_clasico`
- `benchmark_nuestro`

### Opción 2. Compilación manual

```bash
gcc -Wall -Wextra -g -O2 -c editor.c
gcc -Wall -Wextra -g -O2 -c io.c
gcc -Wall -Wextra -g -O2 -c compress.c
gcc -Wall -Wextra -g -O2 -c main.c
gcc -Wall -Wextra -g -O2 -o editor main.o editor.o io.o compress.o -lz
```

## Ejecución

```bash
./editor
```

Menú esperado:

```text
[1]Escribir [2]Borrar [3]Ver [4]Guardar [5]Abrir [6]Renombrar [0]Salir
```

Flujo básico de uso:

1. Elegir `1` para escribir texto.
2. Elegir `3` para visualizar el contenido.
3. Elegir `4` para guardar en un archivo `.cez`.
4. Elegir `5` para abrir el archivo actual.
5. Elegir `6` para cambiar el nombre del archivo.

## Benchmarks

El repositorio incluye dos pruebas comparativas:

- [`benchmark_clasico.c`](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/benchmark_clasico.c): copia un archivo de texto usando `fgetc()` y `fputc()`.
- [`benchmark_nuestro.c`](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/benchmark_nuestro.c): lee el archivo completo, lo comprime y lo escribe con la capa propia de I/O.

Archivo de prueba esperado por ambos benchmarks:

```text
test_50mb.txt
```

Ejemplo de ejecución:

```bash
./benchmark_clasico
./benchmark_nuestro
```

## Conceptos técnicos aplicados

- Estructuras de datos eficientes para edición de texto.
- Gestión manual de memoria con `malloc`, `calloc`, `realloc` y `free`.
- Syscalls de archivos en C.
- Diseño de formatos binarios.
- Verificación de integridad con `CRC32`.
- Compresión de datos con `zlib`.
- Modularización mediante archivos `.c` y `.h`.

## Estado actual del repositorio

Al analizar el código actual se observan algunos puntos importantes:

- `compress.c` y `compress.h` están vacíos en este estado del repositorio.
- `main.c` y `benchmark_nuestro.c` dependen de `compress_data()` y `decompress_data()`, por lo que falta esa implementación para compilar correctamente.
- El `Makefile` referencia `compress.c`, pero además contiene reglas con `$` donde probablemente debería usarse `$^` o `$<`, lo que puede impedir compilar los benchmarks tal como está.
- En este entorno de revisión no estaban instalados `make` ni `gcc`, así que no fue posible validar la compilación localmente desde PowerShell.

Por lo anterior, el README documenta tanto la intención del proyecto como el estado real del código versionado actualmente.

## Posibles mejoras

- Implementar completamente el módulo de compresión.
- Añadir manejo de cursor interactivo más allá del borrado al final.
- Permitir edición de múltiples líneas de forma más cómoda.
- Incorporar pruebas automáticas.
- Agregar mediciones formales de tiempo y uso de memoria para los benchmarks.
- Mejorar la portabilidad entre Linux, macOS y Windows.

## Licencia

No se encontró una licencia definida en el repositorio. Si el proyecto va a publicarse o compartirse externamente, conviene agregar una licencia explícita.
