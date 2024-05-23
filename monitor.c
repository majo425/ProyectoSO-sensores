#include "metodosMonitor.h"

//Declaracion de Variables y semaforos.
// /*--- Definción de semáforos para sincronización entre hilos ---*/
sem_t buffer_mutex;
sem_t temperatura_completa;
sem_t ph_completa;
sem_t recolector_finalizado;
// /*--- Definición de buffers compartidos para guardar las mediciones ---*/
int cont_temperatura;
int cont_ph;
char temperatura_buffer[MAX_BUFFER_SIZE];
char ph_buffer[MAX_BUFFER_SIZE];

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

  printf("\nMONITOR ACTIVO\n");
  printf("Tipo de Sensor: %d\n", buffer);
  printf("Archivo para Escribir la TEMP: %s\n", temp);
  printf("Archivo para Escribir el PH: %s\n", ph);
  printf("Nombre Pipe Nominal: %s\n", pipe_nominal);

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