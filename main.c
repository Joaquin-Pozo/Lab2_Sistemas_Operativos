#include <stdio.h>
#include <stdlib.h> 
#include <string.h>

#define MAX_LINEA 1024

typedef struct {
    char show_id[10];
    char type[50];
    char title[200];
    char director[200];
    char cast[500];
    char country[200];
    char date_added[50];
    int release_year;
    char rating[10];
    char duration[50];
    char listed_in[300];
    char description[500];
} RegistroNetflix;


// Función que elimina las comillas de un string
void limpiarComillas(char *str) {
    // Puntero que recorre el string original, caracter por caracter
    char *src = str; 
    // Puntero que escribe solamente los caracteres del string que no sean comillas
    char *dst = str;
    // Recorre el string hasta que llega al final
    while (*src != '\0') {
        // Si el caracter no es una comilla, lo copia. Caso contrario, lo salta
        if (*src != '"') {
            *dst = *src;
            dst++;
        }
        src++;
    }
    // Marca el fin del string
    *dst = '\0';
}

// Función que parsea una línea CSV respetando las comillas
void parsearLineaCSV(const char *linea, RegistroNetflix *r) {
    // Arreglo que almacena temporalmente los registros del archivo
    char *campos[12];
    // Indice para moverse por el arreglo de campos
    int campoActual = 0;
    // Flag para saber si está dentro de comillas (0: fuera comillas, 1: dentro comillas)
    int dentroComillas = 0;
    char buffer[MAX_LINEA];
    // Indice para moverse por el buffer
    int pos = 0;

    // Recorre cada caracter de la linea hasta que llegue al final o a un salto de linea
    for (int i = 0; linea[i] != '\0' && linea[i] != '\n'; i++) {
        char c = linea[i];

        // Si el caracter actual es una comilla doble, cambia el estado del flag
        if (c == '"') {
            // Flag: alterna el estado
            dentroComillas = !dentroComillas;

        // Si el caracter es una coma y no está dentro de comillas, finaliza de leer ese campo
        } else if (c == ',' && !dentroComillas) {
            // Marca el fin del string y lo almacena en su respectivo campo
            buffer[pos] = '\0';
            campos[campoActual] = strdup(buffer);
            campoActual++;
            pos = 0;
        // Si no es comilla ni coma, copia el caracter al buffer y avanza su posicion
        } else {
            buffer[pos++] = c;
        }
    }
    // Marca el fin del ultimo string y lo almacena en su respectivo campo
    buffer[pos] = '\0';
    campos[campoActual++] = strdup(buffer);

    // Copia cada uno de los campos del arreglo campos al struct RegistroNetflix
    if (campoActual >= 12) {
        strncpy(r->show_id, campos[0], sizeof(r->show_id));
        strncpy(r->type, campos[1], sizeof(r->type));
        strncpy(r->title, campos[2], sizeof(r->title));
        strncpy(r->director, campos[3], sizeof(r->director));
        strncpy(r->cast, campos[4], sizeof(r->cast));
        strncpy(r->country, campos[5], sizeof(r->country));
        strncpy(r->date_added, campos[6], sizeof(r->date_added));
        r->release_year = atoi(campos[7]);
        strncpy(r->rating, campos[8], sizeof(r->rating));
        strncpy(r->duration, campos[9], sizeof(r->duration));
        strncpy(r->listed_in, campos[10], sizeof(r->listed_in));
        strncpy(r->description, campos[11], sizeof(r->description));
    }

    // Limpia comillas en todos los campos
    limpiarComillas(r->show_id);
    limpiarComillas(r->type);
    limpiarComillas(r->title);
    limpiarComillas(r->director);
    limpiarComillas(r->cast);
    limpiarComillas(r->country);
    limpiarComillas(r->date_added);
    limpiarComillas(r->rating);
    limpiarComillas(r->duration);
    limpiarComillas(r->listed_in);
    limpiarComillas(r->description);

    // Liberar memoria del arreglo temporal
    for (int i = 0; i < campoActual; i++) {
        free(campos[i]);
    }
}

// Función que lee un archivo y almacena la información en un arreglo de structs
RegistroNetflix *leerArchivo(const char *nombreArchivo, int *totalRegistros) {
    FILE *archivo = fopen(nombreArchivo, "r");

    // Comprueba que el archivo exista en el mismo path
    if (archivo == NULL) {
        printf("Error al abrir el archivo %s\n", nombreArchivo);
        *totalRegistros = 0;
        return NULL;
    }

    char linea[MAX_LINEA];
    int capacidad = 10;
    *totalRegistros = 0;
    RegistroNetflix *registros = malloc(capacidad * sizeof(RegistroNetflix));

    // Salta la primera línea del encabezado
    fgets(linea, MAX_LINEA, archivo);

    // Lee el archivo hasta que no queden lineas por leer
    while (fgets(linea, MAX_LINEA, archivo)) {
        // Si el arreglo se llena, aumenta su capacidad en un 50%
        if (*totalRegistros >= capacidad) {
            capacidad += capacidad/2;
            registros = realloc(registros, capacidad * sizeof(RegistroNetflix));
        }

        // Crea un puntero al siguiente espacio disponible del arreglo
        RegistroNetflix *r = &registros[*totalRegistros];

        // Lee y almacena en el struct correctamente
        parsearLineaCSV(linea, &registros[*totalRegistros]);
        (*totalRegistros)++;
    }

    fclose(archivo);
    return registros;
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Uso: %s <nombre_archivo.csv>\n", argv[0]);
        return 1;
    }

    int total = 0;
    RegistroNetflix *registros = leerArchivo(argv[1], &total);

    if (registros) {
        printf("Se leyeron %d registros correctamente.\n", total);
        for (int i = 0; i < 100; i++) {
            printf("Registro %i:  Título: %s - Tipo: %s - País: %s (%d)\n", 
                i + 1, registros[i].title, registros[i].type, 
                registros[i].country, registros[i].release_year);
        }
        free(registros);
    }

    return 0;
}