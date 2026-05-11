# Proyecto 3 - Editor CEZ

Editor de archivos en C orientado a conceptos de sistemas operativos. El proyecto propone un editor simple en consola con una estructura de datos tipo **Gap Buffer**, lectura y escritura de archivos con llamadas de bajo nivel tipo POSIX, compresión con `zlib` y un formato binario propio (`.cez`) con metadatos e integridad por `CRC32`.

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

## Introducción y objetivo

Según el informe `Proyecto3_SistemasOperativos_Final (1).docx`, el objetivo principal del proyecto es **demostrar empíricamente que comprimir datos en User Space antes de invocar syscalls del kernel puede generar un ahorro neto de tiempo**, al reducir la carga sobre el bus I/O y la latencia de escritura a disco.

El pipeline planteado es:

1. El usuario edita texto en memoria usando un `Gap Buffer`.
2. Al guardar, el contenido se comprime con `zlib` en User Space.
3. Se construye un `FileHeader` de 64 bytes con metadatos del archivo.
4. El payload comprimido se escribe con syscalls POSIX directas.
5. La escritura se realiza en bloques alineados a `PAGE_SIZE = 4096`.
6. El resultado final es un archivo binario `.cez`, no texto plano.

El informe también propone usar `strace`, `time` y `valgrind` para medir reducción de syscalls, tiempos de ejecución y fugas de memoria.

## Arquitectura del proyecto

### 1. Editor de texto

El módulo [editor.c](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/editor.c) implementa un **Gap Buffer**, una estructura eficiente para inserciones y borrados cerca del cursor.

Funciones principales:

- `gb_create()`: crea el buffer.
- `gb_destroy()`: libera memoria.
- `gb_insert()`: inserta un carácter.
- `gb_delete()`: elimina el carácter anterior al cursor.
- `gb_move_cursor()`: mueve el cursor.
- `gb_get_text()`: obtiene una copia lineal del contenido.

### 2. Entrada/salida de bajo nivel

El módulo [io.c](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/io.c) maneja archivos usando syscalls estilo POSIX:

- `open()`
- `read()`
- `write()`
- `close()`
- `fstat()`

La lectura y escritura se realiza por bloques de `PAGE_SIZE = 4096` bytes.

Decisiones de diseño descritas en el informe:

- Uso de `open/read/write/close` en vez de `stdio` para tener control explícito sobre el buffering.
- Escritura en bloques de 4 KB para alinearse con el tamaño de página del kernel.
- Menor número de syscalls frente a un enfoque carácter por carácter.
- Preferencia por `write()` sobre `mmap()` para un caso de escritura secuencial de tamaño moderado.

### 3. Formato de archivo `.cez`

El encabezado del formato está definido en [format.h](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/format.h).

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

El archivo [main.c](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/main.c) orquesta el flujo del editor:

- Obtiene el texto del `Gap Buffer`.
- Lo comprime antes de guardarlo.
- Construye el encabezado `.cez`.
- Verifica `CRC32` al abrir archivos.
- Descomprime el contenido antes de volverlo a cargar al editor.

## Pipeline de I/O

```text
[Usuario escribe en terminal]
        |
        v
[Gap Buffer en RAM]
        |
        v
[gb_get_text()]
        |
        v
[compress_data()]  -> compresión en User Space
        |
        v
[FileHeader + payload comprimido]
        |
        v
[io_write_file()]  -> open/write/close
        |
        v
[Archivo .cez en disco]
```

## Gestión de memoria

El diseño del proyecto se apoya en memoria dinámica controlada manualmente:

- `gb_create()` reserva el buffer inicial con `calloc`.
- El `Gap Buffer` crece con `realloc` cuando el espacio del gap se agota.
- `gb_get_text()` devuelve una copia contigua del texto para su posterior compresión.
- El flujo de guardado libera explícitamente los buffers temporales después de usarlos.

Ejemplo conceptual del ciclo de vida en guardado:

```c
char *text = gb_get_text(gb);
compress_data(text, ..., &compressed, ...);
free(text);
char *final_buf = malloc(total_size);
free(compressed);
io_write_file(path, final_buf, total_size);
free(final_buf);
```

## Integridad de datos

El proyecto usa `CRC32` sobre el payload comprimido:

- Al guardar, se calcula el checksum y se almacena en el `FileHeader`.
- Al abrir, se recalcula sobre el payload leído.
- Si el valor no coincide, el archivo se marca como corrupto y se aborta la descompresión.

Esto está reflejado en [main.c](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/main.c) y en la definición del encabezado dentro de [format.h](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/format.h).

## Especificación del formato `.cez`

Según el informe, el archivo `.cez` se organiza así:

| Offset | Bytes | Tipo | Descripción |
| --- | --- | --- | --- |
| 0 | 4 | `char[4]` | Magic number `CEZ\0` |
| 4 | 1 | `uint8_t` | Versión del formato |
| 5 | 1 | `uint8_t` | Flags |
| 6 | 2 | `uint16_t` | Tamaño del header |
| 8 | 4 | `uint32_t` | Tamaño original |
| 12 | 4 | `uint32_t` | Tamaño comprimido |
| 16 | 4 | `uint32_t` | `CRC32` |
| 20 | 8 | `uint64_t` | Timestamp Unix |
| 28 | 28 | `char[28]` | Nombre original |
| 56 | 8 | `uint8_t[8]` | Reservado |
| 64 | variable | `Bytef*` | Payload comprimido |

El `header` debe medir exactamente `64` bytes. Por eso el diseño usa `__attribute__((packed))` y una validación con `_Static_assert`.

## Estructura del repositorio

```text
.
|-- main.c
|-- editor.c
|-- editor.h
|-- io.c
|-- io.h
|-- format.h
|-- compress.c
|-- compress.h
|-- benchmark_clasico.c
|-- benchmark_nuestro.c
`-- Makefile
```

## Requisitos

Para compilar el proyecto se necesita lo siguiente:

- Compilador `gcc`
- `make` opcional
- Biblioteca `zlib`
- Un entorno compatible con llamadas POSIX

En Linux o WSL, por ejemplo:

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

- [benchmark_clasico.c](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/benchmark_clasico.c): copia un archivo de texto usando `fgetc()` y `fputc()`.
- [benchmark_nuestro.c](/c:/Users/57318/EAFIT/Semestre_6/Sistemas_operativos/Proyecto3_SistemasOperativos/benchmark_nuestro.c): lee el archivo completo, lo comprime y lo escribe con la capa propia de I/O.

Archivo de prueba esperado por ambos benchmarks:

```text
test_50mb.txt
```

Ejemplo de ejecución:

```bash
./benchmark_clasico
./benchmark_nuestro
```

## Resultados reportados en el informe

El documento `.docx` incluye resultados de benchmarking obtenidos en **Ubuntu 24.04 sobre WSL2** con un archivo de prueba de aproximadamente **53 MB**. Esos resultados reportados son:

| Métrica | Clásico | Nuestro | Impacto |
| --- | --- | --- | --- |
| Volumen a disco | 53 MB | 157 KB | `-99.7%` |
| Llamadas `write()` | 13,429 | 41 | `-99.7%` |
| Llamadas `read()` | 13,430 | 13,430 | Similar |
| Total de syscalls | 26,897 | 13,524 | `-49.7%` |
| Tiempo CPU User | 0.258 s | 0.221 s | `-14.3%` |
| Tiempo Sys | 0.090 s | 0.068 s | `-24.4%` |
| Tiempo real | 0.372 s | 0.306 s | `-17.7%` |
| Memory leaks | N/A | 0 leaks | Sin errores |

Interpretación incluida en el informe:

- La compresión previa reduce drásticamente el volumen de datos enviados al disco.
- El número de `write()` cae de forma muy fuerte, lo que implica menos cruces entre User Space y Kernel Space.
- La reducción del tiempo `sys` respalda que hay menos carga sobre el kernel.
- El costo de compresión queda compensado por la reducción del I/O.

## Evidencia reportada de memoria

El informe incluye un resultado de `valgrind` para `benchmark_nuestro` con:

- `0 bytes in 0 blocks` al salir.
- `8 allocs, 8 frees`.
- `0 errors`.

Eso sugiere que, en la versión evaluada para el informe, no había fugas de memoria detectadas.

## Conceptos técnicos aplicados

- Estructuras de datos eficientes para edición de texto.
- Gestión manual de memoria con `malloc`, `calloc`, `realloc` y `free`.
- Syscalls de archivos en C.
- Diseño de formatos binarios.
- Verificación de integridad con `CRC32`.
- Compresión de datos con `zlib`.
- Modularización mediante archivos `.c` y `.h`.
- Validación en tiempo de compilación con `_Static_assert`.

## Estado actual del repositorio

Al analizar el código actualmente versionado se observan algunos puntos importantes:

- `compress.c` y `compress.h` están vacíos en este estado del repositorio.
- `main.c` y `benchmark_nuestro.c` dependen de `compress_data()` y `decompress_data()`, por lo que falta esa implementación para compilar correctamente.
- El `Makefile` referencia `compress.c`, pero además contiene reglas con `$` donde probablemente debería usarse `$^` o `$<`, lo que puede impedir compilar los benchmarks tal como está.
- En este entorno de revisión no estaban instalados `make` ni `gcc`, así que no fue posible validar la compilación localmente desde PowerShell.

Por eso este README documenta dos cosas a la vez:

- La intención, arquitectura y resultados descritos en el informe `Proyecto3_SistemasOperativos_Final (1).docx`.
- El estado real del código actualmente presente en este repositorio.

Eso importa porque el informe describe una implementación funcional y resultados medidos, mientras que el snapshot actual del repositorio todavía no contiene el módulo de compresión implementado.

## Posibles mejoras

- Implementar completamente el módulo de compresión.
- Añadir manejo de cursor interactivo más allá del borrado al final.
- Permitir edición de múltiples líneas de forma más cómoda.
- Incorporar pruebas automáticas.
- Agregar mediciones formales de tiempo y uso de memoria para los benchmarks.
- Mejorar la portabilidad entre Linux, macOS y Windows.

## Conclusiones del informe

Según el documento entregable, el proyecto permitió demostrar que comprimir en User Space antes de invocar syscalls al kernel puede ser una estrategia efectiva para reducir carga de I/O, número de escrituras y tiempo total del sistema en un escenario de escritura secuencial.

Los puntos destacados del informe son:

- Reducción de `99.7%` en volumen de datos enviados al disco.
- Reducción de `99.7%` en llamadas `write()`.
- Reducción de `49.7%` en total de syscalls.
- Reducción de `24.4%` en tiempo `sys`.
- Reducción de `17.7%` en tiempo real total.
- Uso de un formato binario propio `.cez`.
- Verificación de integridad con `CRC32`.
- Implementación de `Gap Buffer` como base del editor.
- Validación del tamaño exacto del `header` con `_Static_assert`.
