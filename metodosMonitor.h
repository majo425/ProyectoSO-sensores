/*
  TEMA: Simulacion del Monitoreo de Sensores por Mediante Programacion Paralela.
  Autores:
    * Maria José Cárdenas Machaca.
    * Diego Alejandro Albarracin Maldonado.
  Asignatura: Sistemas Operativos.
  Fecha: Primer Semestre de 2024.
  Universidad: Pontificia Universidad Javeriana.
  Descripción: En este proyecto se realiza una simulacion del monitoreo de temperaturas a 
  través de hilos de ejecución y procesos de manera concurrente. La comunicación se lográ por
  medio de pipes nominales. El proyecto nos permitio a poner en práctica los conocimientos 
  adquiridos durante el transcurso de la clase.
*/

#ifndef __METODOSMONITOR_H__
#define __METODOSMONITOR_H__
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

    /*Declaracion de variables haciendo uso de la palabra clave extern 
    para decirle al compilador de que la definicon se encuentra en otro 
    lugar. (monitor.c)*/
    extern sem_t buffer_mutex;
    extern sem_t temperatura_completa;
    extern sem_t ph_completa;
    extern sem_t recolector_finalizado;
    extern int cont_temperatura;
    extern int cont_ph;
    extern char temperatura_buffer[MAX_BUFFER_SIZE];
    extern char ph_buffer[MAX_BUFFER_SIZE];

    /*FIRMA DE LAS FUNCIONES*/
    /*--- Función para abrir un archivo de texto---*/
    FILE *abrirArchivo(char *archivo, char *modoApertura);
    /*--- Función del hilo H-recolector ---*/
    void *H_recolector(char *pipe_nominal);
    /*--- Función del hilo H-ph ---*/
    void *H_ph(char *arch_ph);
    /*--- Función del hilo H-temperatura ---*/
    void *H_temperatura(char *arch_temperatura);
#endif