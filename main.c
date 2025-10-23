#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <pthread.h>

#define MAX_LINEA 1024
#define MAX_PAISES 195
#define MAX_NOMBRE_PAIS 60

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

// Estructura de parámetros para cada hebra
typedef struct {
    RegistroNetflix *registros;
    int inicio;
    int fin; // no inclusivo
    char (*paises)[MAX_NOMBRE_PAIS];
    int *cantidadPaises;
    int *titulos; // arreglo paralelo a paises
} ThreadParams;

// Mutex que protege el acceso a la estructura compartida (paises / titulos)
static pthread_mutex_t mutexPaises;

// Estructura temporal para ordenar y guardar reporte
typedef struct {
    char pais[MAX_NOMBRE_PAIS];
    int cantidad;
} RegistroPais;

/* ----------------- Funciones de limpieza/parsing ----------------- */

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

/* ----------------- Funciones para manejar la tabla de paises (compartida) ----------------- */

// Busca un pais en el arreglo paises. Si lo encuentra devuelve su indice.
// Si no lo encuentra devuelve -1 (no modifica el arreglo).
int buscarIndicePais(char paises[][MAX_NOMBRE_PAIS], int cantidadPaises, const char *pais) {
    for (int i = 0; i < cantidadPaises; i++) {
        // utiliza string compare para ver si son iguales
        if (strcmp(paises[i], pais) == 0) {
            return i;
        }
    }
    return -1;
}

// Inserta un nuevo pais en el arreglo paises en la posición cantidadPaises.
// Devuelve el indice donde se insertó, o -1 si no hay espacio.
int insertarPais(char paises[][MAX_NOMBRE_PAIS], int *cantidadPaises, const char *pais) {
    // verifica si todavia hay espacio en el arreglo
    if (*cantidadPaises >= MAX_PAISES) {
        return -1;
    }
    // copia el nombre del pais dentro del arreglo en la posicion cantidadPaises
    strncpy(paises[*cantidadPaises], pais, MAX_NOMBRE_PAIS - 1);
    paises[*cantidadPaises][MAX_NOMBRE_PAIS - 1] = '\0';
    (*cantidadPaises)++;
    return (*cantidadPaises) - 1;
}

// Busca o inserta el país protegiendo el acceso al arreglo compartido con mutex.
// Devuelve el índice del país (o -1 si no pudo insertar).
int obtenerIndicePaisThreadSafe(char paises[][MAX_NOMBRE_PAIS], int *cantidadPaises, const char *pais) {
    int idx;
    // Evita que otra hebra entre al mismo tiempo
    pthread_mutex_lock(&mutexPaises);
    // Busca si el pais ya existe
    idx = buscarIndicePais(paises, *cantidadPaises, pais);
    if (idx == -1) {
        // Si el pais no existe, lo inserta
        idx = insertarPais(paises, cantidadPaises, pais);
    }
    // Libera el mutex
    pthread_mutex_unlock(&mutexPaises);
    return idx;
}

/* ----------------- Funciones adicionales ----------------- */

// Elimina espacios al inicio y al final (in-place)
void trim_inplace(char *s) {
    // trim a la izq
    char *start = s;
    while (*start == ' ' || *start == '\t') start++;
    if (start != s) memmove(s, start, strlen(start) + 1);

    // trim a la derecha
    size_t len = strlen(s);
    while (len > 0 && (s[len-1] == ' ' || s[len-1] == '\t' || s[len-1] == '\r' || s[len-1] == '\n')) {
        s[len-1] = '\0';
        len--;
    }
}

// Función que extrae el país principal (primer entry antes de la primera coma).
// Copia el resultado en 'dest'. Devuelve 1 si hay país, 0 si no.
int extraerPaisPrincipal(const char *countryField, char *dest, size_t destSize) {
    if (countryField == NULL) {
        return 0;
    }
    char tmp[256];
    // copia el campo country a una variable temporal 'tmp'
    strncpy(tmp, countryField, sizeof(tmp)-1);
    tmp[sizeof(tmp)-1] = '\0';

    // quita las comillas si las hubiera
    limpiarComillas(tmp);

    // buscar primera coma
    char *coma = strchr(tmp, ',');
    // si existe, la reemplaza por '\0', es decir, corta el string
    if (coma) {
        *coma = '\0';
    }

    // Elimina espacios en los bordes
    trim_inplace(tmp);
    // Si el resultado está vacío, devuelve 0
    if (tmp[0] == '\0') {
        return 0;
    }
    // Si el resultado no esta vacio, lo copia en dest y devuelve 1
    strncpy(dest, tmp, destSize-1);
    dest[destSize-1] = '\0';
    return 1;
}

// Función para intercambiar dos elementos
void swap(RegistroPais *a, RegistroPais *b) {
    RegistroPais temp = *a;
    *a = *b;
    *b = temp;
}

// QuickSort recursivo (ordena de mayor a menor)
void quicksort(RegistroPais lista[], int izquierda, int derecha) {
    if (izquierda < derecha) {
        int i = izquierda;
        int j = derecha;
        int pivote = lista[(izquierda + derecha) / 2].cantidad;  // pivote = cantidad central

        while (i <= j) {
            while (lista[i].cantidad > pivote) i++;
            while (lista[j].cantidad < pivote) j--;

            if (i <= j) {
                swap(&lista[i], &lista[j]);
                i++;
                j--;
            }
        }

        // Recursión sobre los dos subarreglos
        if (izquierda < j)
            quicksort(lista, izquierda, j);
        if (i < derecha)
            quicksort(lista, i, derecha);
    }
}


/* ----------------- Worker (hebra) ----------------- */

// Hebra que procesa registros[inicio..fin-1]
void *thread_procesar(void *arg) {
    ThreadParams *p = (ThreadParams *)arg;
    char paisPrincipal[MAX_NOMBRE_PAIS];

    for (int i = p->inicio; i < p->fin; i++) {
        // Extrae el país principal del registro i
        if (!extraerPaisPrincipal(p->registros[i].country, paisPrincipal, sizeof(paisPrincipal))) {
            continue; // si el campo esta vacío, lo ignora y continua
        }

        // Obtiene el indice del país (thread-safe). Si no existe, lo inserta
        int idx = obtenerIndicePaisThreadSafe(p->paises, p->cantidadPaises, paisPrincipal);
        if (idx < 0) {
            // Si no se pudo insertar (quizas por arreglo lleno), omite y continua
            continue;
        }

        // Bloqua el mutex
        pthread_mutex_lock(&mutexPaises);
        // Se incrementa el contador en su indice correspondiente
        p->titulos[idx]++;
        // Desbloquea el mutex
        pthread_mutex_unlock(&mutexPaises);
    }

    return NULL;
}

/* ----------------- Orquestador: crea hebras y genera reporte ----------------- */

// Función que recibe registros y divide el trabajo entre cantidad_hebras,
// actualiza los arreglos paises/titulos y escribe el archivo de salida.
void conteoConHebras(RegistroNetflix *registros, int lineas, int cantidad_hebras) {
    // Arreglos compartidos (dentro de esta función pero accesibles por threads via params)
    static char paises[MAX_PAISES][MAX_NOMBRE_PAIS];
    static int titulos[MAX_PAISES];

    // inicializar
    int cantidadPaises = 0;
    for (int i = 0; i < MAX_PAISES; i++) {
        titulos[i] = 0;
    }
    for (int i = 0; i < MAX_PAISES; i++) {
        paises[i][0] = '\0';
    }

    // Inicializa el mutex
    pthread_mutex_init(&mutexPaises, NULL);

    // Crea los parametros y threads
    pthread_t *threads = malloc(sizeof(pthread_t) * cantidad_hebras);
    ThreadParams *params = malloc(sizeof(ThreadParams) * cantidad_hebras);

    // base = cantidad minimima de lineas que recibe cada hebra
    int base = lineas / cantidad_hebras;
    // resto = cantidad de hebras extras que se deben procesar
    int resto = lineas % cantidad_hebras;
    int inicio = 0;

    // Realiza una distribución equitativa de lineas entre hebras
    for (int i = 0; i < cantidad_hebras; i++) {
        // Si i es menor que el resto, le suma 1 a la base, caso contrario, no suma nada
        int cantidad_lineas = base + (i < resto ? 1 : 0);
        params[i].registros = registros;
        params[i].inicio = inicio;
        params[i].fin = inicio + cantidad_lineas;
        params[i].paises = paises;
        params[i].cantidadPaises = &cantidadPaises;
        params[i].titulos = titulos;

        // crear hebra
        pthread_create(&threads[i], NULL, thread_procesar, &params[i]);

        inicio += cantidad_lineas;
    }

    // Esperar todas las hebras
    for (int i = 0; i < cantidad_hebras; i++) {
        pthread_join(threads[i], NULL);
    }

    // Destruir mutex
    pthread_mutex_destroy(&mutexPaises);

    // Crea un arreglo auxiliar para ordenar por cantidad (mayor a menor)
    RegistroPais *lista = malloc(sizeof(RegistroPais) * cantidadPaises);
    for (int i = 0; i < cantidadPaises; i++) {
        strncpy(lista[i].pais, paises[i], MAX_NOMBRE_PAIS - 1);
        lista[i].pais[MAX_NOMBRE_PAIS - 1] = '\0';
        lista[i].cantidad = titulos[i];
    }

    // Ordena la lsita de paises de manera descendiente
    quicksort(lista, 0, cantidadPaises - 1);

    // Escribir archivo de salida
    FILE *out = fopen("reporte_paises_netflix.txt", "w");
    if (!out) {
        printf("Error creando archivo de salida\n");
    } else {
        fprintf(out, "País principal  | Títulos\n--------------------------------\n");
        for (int i = 0; i < cantidadPaises; i++) {
            fprintf(out, "%s  | %d\n", lista[i].pais, lista[i].cantidad);
        }
        fclose(out);
    }

    // Liberar recursos
    free(lista);
    free(threads);
    free(params);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        printf("Uso: %s <nombre_archivo.csv> <cantidad_hebras>\n", argv[0]);
        return 1;
    }

    int lineas = 0;
    RegistroNetflix *registros = leerArchivo(argv[1], &lineas);

    if (registros == NULL || lineas == 0) {
        printf("El archivo se encuentra vacío o no existe.\n");
    }

    int cantidad_hebras = atoi(argv[2]);
    // utiliza una hebra por defecto, si se ingresa un numero menor a 1
    if (cantidad_hebras <= 0) {
        cantidad_hebras = 1;
    }
    // evita crear más hebras que líneas
    if (cantidad_hebras > lineas) {
        cantidad_hebras = lineas;
    }

    // Llamar la función que orquesta el conteo con hebras
    conteoConHebras(registros, lineas, cantidad_hebras);


    free(registros);

    return 0;
}