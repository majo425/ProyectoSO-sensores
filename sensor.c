#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

// Tamaño del buffer para la lectura del archivo
#define BUFFERSIZE 50

/*--- Estructura con los datos a enviar al monitor ---*/
typedef struct {
  int tipo_sensor;
  float medicion;
} DatosSensor;

/*--- Estructura con los argumentos para la simulación del sensor ---*/
typedef struct {
  int tipo_sensor;    // Tipo de retorno: 1-Temperatura, 2-PH
  int tiempo;         // Tiempo en enviar la medición del sensor al monitor
  char *archivo;      // Archivo con medidas de temperatura o PH
  char *pipe_nominal; // Permite la comunicación entre los procesos
} SensorArgumentos;

/*--- Función para abrir un archivo de texto---*/
FILE *abrirArchivo(char *archivo, char *modoApertura) {
  FILE *arch = fopen(archivo, modoApertura); // Apertura del archivo
  if (!arch) {                               // Manejo de errores en la apertura
    perror("\nError al abrir el archivo\n");
    exit(1);
  } else { // Manejo de apertura éxitosa
    printf("\nArchivo abierto\n");
    return arch;
  }
}

/*--- Función para abrir un pipe ---*/
int abrirPipe(char *pipe_nominal, int modoApertura) {
  int pipe = open(pipe_nominal, modoApertura); // Apertura del pipe
  if (pipe < 0) { // Manejo de errores en la apertura
    perror("\nError al abrir el pipe\n");
    exit(1);
  } else { // Manejo de apertura éxitosa
    printf("\nPipe abierto\n");
    return pipe;
  }
}

/*--- Función para simular la lectura de mediciones y enviarlas al monitor ---*/
void simularSensor(int tipo_sensor, int tiempo, char *archivo, char *pipe_nominal) {
  printf("\nIniciando simulación del sensor\n");

  int pipe = abrirPipe(pipe_nominal, O_WRONLY); // Abrir pipe en modo escritura
  FILE *archivo_leido = abrirArchivo(archivo, "r"); // Abrir archivo modo lectura
  char buffer[BUFFERSIZE];

  // Inicializar la estructura
  DatosSensor datos;
  datos.tipo_sensor = tipo_sensor;

  while (fgets(buffer, BUFFERSIZE, archivo_leido)) { // Lectura del archivo
    
    datos.medicion = atof(buffer);
    printf("\nLectura del archivo: %.2f\n", datos.medicion);

    // Enviar la estructura con el tipo de medición y el valor de la medición
    write(pipe, &datos, sizeof(DatosSensor));
    sleep(tiempo); // Espera un tiempo
  }

  fclose(archivo_leido); // Cierre del archivo
  close(pipe);           // Cierre del pipe
  exit(EXIT_SUCCESS);
  printf("\nSimulación del sensor completada\n");
}

int main(int argc, char *argv[]) {
  // Inicializar la estructura con los parámetros de entrada
  SensorArgumentos args = {.tipo_sensor = -1, .tiempo = -1, .archivo = NULL, .pipe_nominal = NULL};

  char *flags[] = {"-s", "-t", "-f", "-p"};
  int cant_flags = sizeof(flags) / sizeof(flags[0]);

  for (int i = 1; i < argc; i++) { // Recorrido del argumento recibido
    for (int j = 0; j < cant_flags; j++) { // Buscar el argumento en el arreglo
      if (strcmp(argv[i], flags[j]) == 0) { // Asigna el argumento a la variable propia de la estructura
        if (i + 1 < argc) {
          switch (j) {
          case 0:
            args.tipo_sensor = atoi(argv[i + 1]);
            break;
          case 1:
            args.tiempo = atoi(argv[i + 1]);
            break;
          case 2:
            args.archivo = argv[i + 1];
            break;
          case 3:
            args.pipe_nominal = argv[i + 1];
            break;
          }
          i++; // Pasar al siguiente argumento (valor del flag)
        }
        break;
      }
    }
  }

  // Validar que se hayan proporcionado todos los argumentos requeridos
  if (args.tipo_sensor == -1 || args.tiempo == -1 || args.archivo == NULL || args.pipe_nominal == NULL) {
    fprintf(stderr, "Faltan argumentos obligatorios\n");
    fprintf(stderr, "Uso: %s -s tipo_sensor -t tiempo -f archivo -p pipe_nominal\n", argv[0]);
    exit(1);
  }

  // Asignación de los argumentos
  int tipo_sensor = args.tipo_sensor;
  int tiempo = args.tiempo;
  char *archivo = args.archivo;
  char *pipe_nominal = args.pipe_nominal;

  // Impresiones de prueba
  printf("Tipo de Sensor: %d\n", tipo_sensor);
  printf("Intervalo de Tiempo: %d\n", tiempo);
  printf("Archivo: %s\n", archivo);
  printf("Pipe Nominal: %s\n", pipe_nominal);

  // Crear un proceso hijo
  pid_t pid = fork();

  // Itera el numero de sensores ingresados mediante consola
  for (int i = 0; i < argc - 1; i += 4) {
    if (pid == -1) {
      perror("fork");
      exit(EXIT_FAILURE);  // Manejo y salida en caso de un error
    } else if (pid == 0) { // Ejecución del proceso hijo
      simularSensor(tipo_sensor, tiempo, archivo, pipe_nominal);
      exit(EXIT_SUCCESS); // Salir del proceso hijo después de simular el sensor
    }
  }

  // Esperar a que todos los procesos hijos terminen
  for (int i = 0; i < argc - 1; i += 4) {
    wait(NULL);
  }

  return 0;
}