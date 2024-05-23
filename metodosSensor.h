#ifndef __METODOSSENSOR_H__
#define __METODOSSENSOR_H__

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
    FILE *abrirArchivo(char *archivo, char *modoApertura);
    /*--- Función para abrir un pipe ---*/
    int abrirPipe(char *pipe_nominal, int modoApertura);
    /*--- Función para simular la lectura de mediciones y enviarlas al monitor ---*/
    void simularSensor(int tipo_sensor, int tiempo, char *archivo, char *pipe_nominal);

#endif
