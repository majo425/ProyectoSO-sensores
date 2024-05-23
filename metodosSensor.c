#include "metodosSensor.h"

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
