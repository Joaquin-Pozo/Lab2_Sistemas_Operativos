# Laboratorio N°2 - Sistemas Operativos
### Universidad de Santiago de Chile
### Estudiante: Joaquín Pozo
### Rut: 20.237.059-4
#### Ejecutado y compilado en Ubuntu 22.04.5 LTS

En esta tarea de programación, se requiere implementar un programa en C que procese un archivo de datos real del catálogo de Netflix, descargado desde Kaggle.

El objetivo principal es contar cuántos títulos pertenecen a cada país principal, utilizando múltiples hebras (pthreads) para paralelizar el procesamiento del archivo y un mutex para garantizar la correcta sincronización al actualizar los contadores compartidos. Para esta tarea, se define país principal como el primer país listado en la columna country (si hay múltiples países separados por comas, se toma el que aparece antes de la primera coma; si country está vacío o NULL, se ignora esa fila).

Para compilar el programa se debe ejecutar la siguiente línea de código en la misma dirección del archivo 'main.c':

```bash
gcc main.c -o main
```

Para ejecutar el programa se debe ingresar el nombre del archivo con su extensión (archivo de prueba utilizado: netflix_titles.csv) y la cantidad de hebras a utilizar.
Por ejemplo: 

```bash
./main netflix_titles.csv 10
```

El programa generará un archivo "reporte_paises_netflix.txt" con los paises y sus titulos ordenados de manera descendente.