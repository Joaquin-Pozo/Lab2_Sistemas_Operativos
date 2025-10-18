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

RegistroNetflix *leerArchivoCSV(const char *nombreArchivo, int *totalRegistros) {
    FILE *archivo = fopen(nombreArchivo, "r");
    if (!archivo) {
        printf("Error al abrir el archivo %s\n", nombreArchivo);
        *totalRegistros = 0;
        return NULL;
    }

    char linea[MAX_LINEA];
    int capacidad = 10;
    *totalRegistros = 0;
    RegistroNetflix *registros = malloc(capacidad * sizeof(RegistroNetflix));

    // Saltar la primera línea (encabezado)
    fgets(linea, MAX_LINEA, archivo);

    while (fgets(linea, MAX_LINEA, archivo)) {
        if (*totalRegistros >= capacidad) {
            capacidad *= 2;
            registros = realloc(registros, capacidad * sizeof(RegistroNetflix));
        }

        RegistroNetflix *r = &registros[*totalRegistros];

        // Nota: strtok separa por coma, pero este CSV tiene comillas, se maneja de forma básica
        sscanf(linea, "%9[^,],%49[^,],%199[^,],%199[^,],%499[^,],%199[^,],%49[^,],%d,%9[^,],%49[^,],%299[^,],%499[^\n]",
               r->show_id, r->type, r->title, r->director, r->cast, r->country,
               r->date_added, &r->release_year, r->rating, r->duration,
               r->listed_in, r->description);

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
    RegistroNetflix *registros = leerArchivoCSV(argv[1], &total);

    if (registros) {
        printf("Se leyeron %d registros correctamente.\n", total);
        for (int i = 0; i < total; i++) {
            printf("Título: %s - Tipo: %s - País %s (%d)\n", registros[i].title, registros[i].type, registros[i].country, registros[i].release_year);
        }
        free(registros);
    }

    return 0;
}