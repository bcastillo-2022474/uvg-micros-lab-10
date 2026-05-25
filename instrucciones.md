# Universidad del Valle de Guatemala

## Facultad de Ingeniería

### Departamento de Ciencias de la Computación

**CC3086 Programación de Microprocesadores**
*Ciclo 1 de 2026*

# Laboratorio 10

## Sincronización con Variables de Condición & Semáforos

---

## Objetivo

Implementar un programa que utilice `pthreads` y mecanismos de sincronización en C++ que desarrolle aplicaciones reales de implementación.

---

## Instrucciones

El desarrollo del laboratorio se llevará a cabo durante la semana 18, en grupos de 2 personas.

Recuerda para compilar y ejecutar programas en WSL o similares:

### Compilar programas en C++

```bash
g++ -Wall -Wextra -pthread nombre_archivo.cpp -o nombre_ejecutable -lz
```

### Ejecutar

```bash
./nombre_ejecutable
```

---

## Descripción de los parámetros

* `-Wall`: activa advertencias comunes del compilador.
* `-Wextra`: activa advertencias adicionales para detectar posibles errores.
* `-pthread`: habilita el soporte para hilos POSIX (Pthreads).
* `-lz`: enlaza la librería Zlib utilizada para compresión y descompresión de datos.

---

# Compresión de archivos en paralelo

## I. Antes de iniciar a desarrollar el laboratorio

### a) Instalación de dependencias

Ejecuta los siguientes comandos en el entorno WSL Ubuntu para instalar la librería `zlib`.

*(La imagen/comando original no estaba disponible en el documento extraído.)*

La instalación mostrará un resultado similar al siguiente.

---

### b) Creación del archivo de prueba

Copia el archivo `input.txt` compartido en Canvas y guárdalo en el directorio de tu preferencia.

---

### c) Prueba de funcionamiento inicial

Copia el programa llamado `compression.cpp` compartido en Canvas y pégalo en el directorio de tu preferencia dentro del entorno WSL Ubuntu (preferiblemente en el mismo directorio donde se sitúa `input.txt`).

El programa realiza lo siguiente:

* Lee un archivo de entrada (`input.txt`) completamente en memoria.
* Comprime el contenido utilizando la función `compress()` de la librería `zlib` (algoritmo DEFLATE), el mismo empleado por herramientas como `gzip` y `zip`.
* Guarda el resultado en un archivo de salida (`output.bin`).

---

### d) Verificación del tamaño

Comprueba el tamaño del archivo original en Bytes.

El sistema mostrará el tamaño exacto del archivo en Bytes, por ejemplo, `372`.

---

### e) Compilación y ejecución del programa

Compila y ejecuta el programa de compresión con:

```bash
g++ -Wall -Wextra -pthread compression.cpp -o compression -lz
./compression
```

El resultado esperado será un mensaje que indique:

* El archivo se comprimió exitosamente.
* Tamaño del archivo original en Bytes.
* Tamaño del archivo comprimido en Bytes.

---

# II. Información preliminar

Cuando se comprime un archivo grande (utilizando algoritmos como Huffman, LZ77 o DEFLATE), este debe recorrerse y dividirse en bloques para ser transformado. Si el archivo es demasiado extenso, comprimirlo de manera secuencial genera un proceso lento e ineficiente.

## DEFLATE

DEFLATE es un algoritmo de compresión sin pérdida que combina dos técnicas principales:

* **LZ77 (Sliding Window Dictionary):** busca repeticiones de secuencias de bytes dentro de una ventana y las reemplaza por referencias (distancia + longitud).
* **Codificación de Huffman:** asigna códigos más cortos a los símbolos frecuentes, optimizando el espacio de almacenamiento.

---

# III. Actividad práctica

En esta práctica, el estudiante deberá dividir el archivo de entrada (`paralelismo_teoria.txt`) en bloques de tamaño fijo (por ejemplo, de `1 MB` cada uno).

Se debe justificar la elección del tamaño de bloque en función del rendimiento esperado y la eficiencia del proceso.

Al iniciar el programa, se deberá presentar un menú inicial con al menos dos opciones:

```text
1. Comprimir archivo
2. Descomprimir archivo previamente comprimido
```

El usuario podrá elegir la opción deseada.

---

## a) Compresión

### i.

Ingresar por teclado cuántos hilos se utilizarán para realizar el proceso.

### ii.

Asignar bloques a hilos POSIX (`pthread_create`) de forma que cada hilo comprima un bloque de manera independiente.

### iii.

Sincronizar la escritura de resultados en el archivo comprimido final.

Para esto, debes seleccionar y aplicar los mecanismos de sincronización que consideres más adecuados (`mutex`, variables de condición, semáforos).

### iv.

Garantizar el orden correcto de los bloques en el archivo de salida, asegurando que la secuencia del texto comprimido mantenga la coherencia con el archivo original.

### v.

Medir tiempos y comparar tiempos de ejecución utilizando distintos números de hilos (ejemplo: `1, 2, 4, 8...100`) y contrastarlos con la versión secuencial.

---

## b) Descompresión

### i.

Ingresar por teclado cuántos hilos se utilizarán para realizar el proceso.

### ii.

Asignar bloques a hilos POSIX (`pthread_create`) de forma que cada hilo descomprima un bloque del archivo previamente comprimido.

### iii.

Sincronizar la escritura del contenido descomprimido en el archivo de salida, asegurando que no ocurran condiciones de carrera.

### iv.

Garantizar el orden correcto de los bloques descomprimidos en el archivo de salida, de manera que el resultado coincida exactamente con el archivo original.

### v.

Verificar la integridad del archivo descomprimido comparándolo con el archivo de entrada original (`paralelismo_teoria.txt`).

---

# IV. Análisis experimental y comportamiento del sistema

El grupo deberá ejecutar el programa utilizando distintos números de hilos (por ejemplo: `1, 2, 4 y 8`) y registrar los tiempos de ejecución obtenidos durante el proceso de encriptación y desencriptación.

## Calcular el speedup

Speedup = \frac{T_{secuencial}}{T_{paralelo}}

## Calcular la eficiencia

Eficiencia = \frac{Speedup}{N_{hilos}}

Además:

* Analizar si el incremento en el número de hilos realmente mejora el rendimiento del programa o si existe un punto en el que el desempeño deja de mejorar.
* Realizar pruebas utilizando distintos tamaños de bloque (por ejemplo: `256 KB`, `1 MB` y `4 MB`) y comparar:

  * tiempo de ejecución
  * número de hilos
  * speedup
  * eficiencia

---

# V. Entregables

## Código fuente

* Versión paralela con Pthreads y mecanismos de sincronización.

---

## Documento breve (2–3 páginas)

* Explicación de los mecanismos de sincronización utilizados y justificación de su implementación en este código.
* Explicación de la lógica de paralelización utilizada para dividir el archivo y asignar bloques a hilos POSIX.
* Comparación de tiempos entre la versión secuencial y paralela utilizando distintos números de hilos.
* Análisis del speedup y eficiencia obtenidos durante las pruebas realizadas.
* Tabla comparativa con:

  * número de hilos
  * tiempo de ejecución
  * speedup
  * eficiencia
  * tamaño de bloque utilizado
* Inclusión de al menos una gráfica comparativa de tiempos de ejecución.

---

## Video (duración aproximada: 6 min)

* Demostración del programa realizando encriptación y desencriptación paralela.
* Explicación breve de la lógica de paralelización implementada.
* Justificación de los mecanismos de sincronización utilizados y del recurso compartido protegido.
* Explicación de qué ocurriría si no existiera sincronización entre hilos.
* Presentación de resultados utilizando distintos números de hilos y tamaños de bloque.
* Explicación del punto donde el rendimiento dejó de mejorar al aumentar los hilos.
* Explicación de cómo se verificó que el archivo desencriptado coincide con el original.

---

Fuente: 

