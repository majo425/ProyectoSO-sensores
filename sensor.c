#include "metodosSensor.h"

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