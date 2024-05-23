#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

// Tamaño máximo del buffer
#define MAX_BUFFER_SIZE 50

/*--- Estrcutura de datos con los argumentos para el proceso monitor ---*/
typedef struct {
  int tam_buffer; // Tamaño de los buffer donde se colocaran las medidas
  char *arch_temperatura; // Archivo para guardar mediciones de temperatura
  char *arch_ph;          // Archivo para guardar mediciones de ph
  char *pipe_nominal;     // Permite la comunicación entre los procesos
} MonitorArgumentos;

/*--- Estructura de datos para descomponer el mensaje recibido ---*/
typedef struct {
  int tipo_sensor; // Tipo de sensor: 1 para temperatura, 2 para pH
  float medicion;  // Medición del sensor
} DatosSensor;

/*--- Definición de buffers compartidos para guardar las mediciones ---*/
float temperatura_buffer[MAX_BUFFER_SIZE]; // Almacenar mediciones de temperatura
float ph_buffer[MAX_BUFFER_SIZE];        // Alamacenar mediciones de ph
int cont_temperatura = 0; // Contador de mediciones de temperatura
int cont_ph = 0;          // Contador de mediciones de ph

/*--- Definción de semáforos para sincronización entre hilos ---*/
sem_t buffer_mutex;          // Garantizar la exclusión mutua
sem_t temperatura_completa;  // Indicar que está lleno el buffer de temperatura
sem_t ph_completa;           // Indicar que está lleno el buffer de ph
sem_t recolector_finalizado; // Indicar que el hilo recolector ha finalizado

/*--- Función para abrir un archivo de texto---*/
FILE *abrirArchivo(char *archivo, char *modoApertura) {
  FILE *arch = fopen(archivo, modoApertura); // Apertura del archivo
  if (!arch) {                               // Manejo de errores en la apertura
    perror("\nError al abrir el archivo\n");
    exit(1);
  } else { // Manejo de apertura éxitosa
    printf("\nArchivo abierto\n");
    return arch; // Retorno del puntero del archivo abierto
  }
}

/*--- Función del hilo H-recolector ---*/
void *H_recolector(char *pipe_nominal) {
  // Crea el pipe si no existe
  if (mkfifo(pipe_nominal, 0666) == -1) {
    if (errno != EEXIST) {
      perror("mkfifo");
      exit(1);
    }
  }
  // Abrir el pipe en modo lectura
  int pipe = open(pipe_nominal, O_RDONLY);
  if (pipe == -1) {
    perror("\nError al abrir el pipe en modo lectura\n");
    exit(1);
  }
 
  // Leer medidas del pipe y colocarlas en el buffer correspondiente
  while (1) {
    DatosSensor datosSensor;
    if (read(pipe, &datosSensor, sizeof(DatosSensor)) == sizeof(DatosSensor)) { // Lectura del pipe
      if (datosSensor.tipo_sensor == 1) { // Si la medición es de tipo temperatura se valida
        float medicionTemp = datosSensor.medicion; // Obtener la medición de temperatura
        if (medicionTemp >= 20.0 && medicionTemp <= 31.6) { // Validar que esté dentro del rango
          sem_wait(&buffer_mutex);
          if (cont_temperatura < MAX_BUFFER_SIZE) {
            temperatura_buffer[cont_temperatura] = medicionTemp;   // Guardar la medición
            cont_temperatura++;
            sem_post(&temperatura_completa); // Indicar que se ha guardado la medición
          }
          sem_post(&buffer_mutex); // Permitir el acceso a otro proceso
        } else {
          printf("\nMedida de temperatura fuera de rango descartada: %.2f\n",medicionTemp);
        }
      } else if (datosSensor.tipo_sensor == 2) { // Si la medición es de tipo Ph se valida
        float medicionPh = datosSensor.medicion; // Obtener la medición de ph
        if (medicionPh >= 6.0 && medicionPh <= 8.0) {   // Validar que esté dentro del rang
          sem_wait(&buffer_mutex);
          if (cont_ph < MAX_BUFFER_SIZE) {
            ph_buffer[cont_ph] = medicionPh; // Guardar la medición
            cont_ph++;     
            sem_post(&ph_completa); // Indicar que se ha guardado la medición
          }
          sem_post(&buffer_mutex); // Permitir el acceso a otro proceso
        } else {
          printf("\nMedida de ph fuera de rango descartada: %.2f", medicionPh);
        }
      } else {
        printf("\nTipo de sensor desconocido: %d\n", datosSensor.tipo_sensor);
      }
    } else {
      perror("\nError al leer desde el pipe\n");
      break;
    }
  }
  if(close(pipe) == -1){ // Cerrar el pipe
    perror("\nError al cerrar el pipe\n");
  }else{
    printf("\nSe cerro el pipe\n");
  }
  printf("\nHilo recolector ha finalizado.\n");
  //sem_post(&recolector_finalizado); // Indicar que el recolector ha finalizado
  return NULL;
}

/*--- Función del hilo H-ph ---*/
void *H_ph(char *arch_ph) {
  FILE *archivoPhFinal =
      abrirArchivo(arch_ph, "a"); // Abrir archivo modo escritura

  while (1) {
    sem_wait(&ph_completa); 
    sem_wait(&buffer_mutex);

    // Obtener la última medicion de ph del buffer
    float valorPh = ph_buffer[cont_ph - 1];
    printf("\nValor ph:%.2f", valorPh);

    // Obtener el tiempo actual de guardado en el archivo
    time_t horaActual;
    time(&horaActual);

    // Escribir en el archivo la medición y la hora de guardado
    fprintf(archivoPhFinal, "%.2f (hora de guardado: %s)\n", valorPh, asctime(localtime(&horaActual)));
    fflush(archivoPhFinal); // Forzar la escritura de los datos

    sem_post(&buffer_mutex); // Permitir el acceso a otro proceso
  }
  fclose(archivoPhFinal); // Cerrar el archivo de texto
  return NULL;
}

/*--- Función del hilo H-temperatura ---*/
void *H_temperatura(char *arch_temperatura) {
  FILE *archivoTempFinal =
      abrirArchivo(arch_temperatura, "a"); // Abrir archivo modo escritura

  while (1) {
    sem_wait(&temperatura_completa);
    sem_wait(&buffer_mutex);

    // Obtener la última medicion de temperatura del buffer
    float valorTemp = temperatura_buffer[cont_temperatura - 1];
    printf("\nValor temperatura:%.2f", valorTemp);

    // Obtener el tiempo actual de guardado en el archivo
    time_t horaActual;
    time(&horaActual);

    // Escribir en el archivo con la hora actual
    fprintf(archivoTempFinal, "%.2f (hora de guardado: %s)\n", valorTemp, asctime(localtime(&horaActual)));
    fflush(archivoTempFinal); // Forzar la escritura de los datos

    sem_post(&buffer_mutex);      // Permitir el acceso a otro proceso
  }
  fclose(archivoTempFinal); // Cerrar el archivo de texto
  return NULL;
}

int main(int argc, char *argv[]){
  // Inicializar la estructura con los parámetros de entrada
  MonitorArgumentos args = {.tam_buffer = -1, .arch_temperatura = NULL, .arch_ph = NULL, .pipe_nominal = NULL};
  // Definir las flags
  char *flags[] = {"-b", "-t", "-h", "-p"};
  int cant_flags = sizeof(flags) / sizeof(flags[0]);

  for (int i = 1; i < argc; i++) {         // Recorrido del argumento recibido
    for (int j = 0; j < cant_flags; j++) { // Buscar el argumento en el arreglo
      if (strcmp(argv[i], flags[j]) == 0) {
        if (i + 1 <
            argc) { // Asigna el argumento a la variable propia de la estructura
          switch (j) {
          case 0:
            args.tam_buffer = atoi(argv[i + 1]);
            break;
          case 1:
            args.arch_temperatura = argv[i + 1];
            break;
          case 2:
            args.arch_ph = argv[i + 1];
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
  if (args.tam_buffer == -1 || args.arch_temperatura == NULL ||
      args.arch_ph == NULL || args.pipe_nominal == NULL) {
    fprintf(stderr, "Faltan argumentos obligatorios\n");
    fprintf(stderr,
            "Uso: %s -b tamaño_buffers -t archivo_temperatura -h archivo_ph -p "
            "pipe_nominal\n",
            argv[0]);
    exit(1);
  }

  int buffer = args.tam_buffer;
  char *temp = args.arch_temperatura;
  char *ph = args.arch_ph;
  char *pipe_nominal = args.pipe_nominal;

  printf("Tipo de Sensor: %d\n", buffer);
  printf("Intervalo de Tiempo: %s\n", temp);
  printf("Archivo: %s\n", ph);
  printf("Pipe Nominal: %s\n", pipe_nominal);

  // Inicialización de semaforos
  sem_init(&buffer_mutex, 0, 1);
  sem_init(&temperatura_completa, 0, 0);
  sem_init(&ph_completa, 0, 0);
  sem_init(&recolector_finalizado, 0, 0);

  // Creación de hilos
  pthread_t recolector_tid, ph_tid, temperatura_tid;

  pthread_create(&recolector_tid, NULL, (void *(*)(void *))H_recolector,(void *)pipe_nominal);
  pthread_create(&ph_tid, NULL, (void *(*)(void *))H_ph, (void *)ph);
  pthread_create(&temperatura_tid, NULL, (void *(*)(void *))H_temperatura, (void *)temp);

  sem_wait(&recolector_finalizado); // Esperar a que finalice el recolector

  // Esperar a que finalicen la ejecución de los hilos
  pthread_join(ph_tid, NULL);
  pthread_join(temperatura_tid, NULL);

  // Destruir los semaforos
  sem_destroy(&buffer_mutex);
  sem_destroy(&temperatura_completa);
  sem_destroy(&ph_completa);
  sem_destroy(&recolector_finalizado);
  unlink(pipe_nominal); // Eliminar el pipe

  return 0;
}