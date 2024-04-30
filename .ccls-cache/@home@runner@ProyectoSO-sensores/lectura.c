#include <stdio.h>
#include <stdlib.h>

// Función para abrir un archivo y leer su contenido línea por línea
void test_read_file(char* filename) {
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error al abrir el archivo");
        exit(EXIT_FAILURE);
    }

    char buffer[128];  // Asumiendo que una línea no excederá los 128 caracteres
    int line_count = 0;

    printf("Iniciando la lectura del archivo: %s\n", filename);

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        printf("Línea %d: %s", ++line_count, buffer);
    }

    if (feof(file)) {  // Verifica si se alcanzó el final del archivo
        printf("Fin de la lectura del archivo. Total de líneas leídas: %d\n", line_count);
    } else {
        perror("Error durante la lectura del archivo");
    }

    fclose(file);
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Uso: %s <archivo>\n", argv[0]);
        return EXIT_FAILURE;
    }

    test_read_file(argv[1]);

    return EXIT_SUCCESS;
}