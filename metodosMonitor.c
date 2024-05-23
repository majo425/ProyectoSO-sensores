// #include <errno.h>
// #include <fcntl.h>
// #include <pthread.h>
// #include <semaphore.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <sys/stat.h>
// #include <sys/types.h>
// #include <time.h>
// #include <unistd.h>
#include "metodosMonitor.h"

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
  //Crea el pipe si no existe
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
    } else if (read(pipe, &datosSensor, sizeof(DatosSensor)) == sizeof(DatosSensor) == -1){
      perror("\nError al leer desde el pipe\n");
      break;
    }else{
      /*Continuar escuchando.*/
      continue;
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