#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

// Tamaño máximo del buffer
#define MAX_BUFFER_SIZE 10

// Estructura para las mediciones
typedef struct {
    float value;
    time_t timestamp;
} Measurement;

// Argumentos para los hilos
typedef struct {
    int buffer_size;
    char* temp_file;
    char* ph_file;
    char* pipe_name;
} MonitorArgs;

// Buffer compartido para las mediciones
Measurement temperature_buffer[MAX_BUFFER_SIZE];
Measurement ph_buffer[MAX_BUFFER_SIZE];
int temperature_count = 0;
int ph_count = 0;

// Semáforos para sincronización entre hilos
sem_t buffer_mutex;
sem_t temperature_full;
sem_t temperature_empty;
sem_t ph_full;
sem_t ph_empty;

// Función del hilo H-recolector
void* collector_thread(void* arg) {
    MonitorArgs* args = (MonitorArgs*)arg;

    // Abrir el pipe en modo lectura
    int pipe_fd = open(args->pipe_name, O_RDONLY);
    if (pipe_fd == -1) {
        perror("Error al abrir el pipe en modo lectura");
        exit(1);
    }

    // Leer medidas del pipe y colocarlas en el buffer correspondiente
    while (1) {
        float measurement;
        if (read(pipe_fd, &measurement, sizeof(float)) > 0) {
            if (measurement > 0) {
                // Determinar el tipo de medida y colocarla en el buffer correspondiente
                if (measurement >= 20.0 && measurement <= 31.0) {
                    sem_wait(&temperature_empty);
                    sem_wait(&buffer_mutex);
                    temperature_buffer[temperature_count].value = measurement;
                    time(&temperature_buffer[temperature_count].timestamp);
                    temperature_count++;
                    sem_post(&buffer_mutex);
                    sem_post(&temperature_full);
                } else if (measurement >= 6.0 && measurement <= 8.0) {
                    sem_wait(&ph_empty);
                    sem_wait(&buffer_mutex);
                    ph_buffer[ph_count].value = measurement;
                    time(&ph_buffer[ph_count].timestamp);
                    ph_count++;
                    sem_post(&buffer_mutex);
                    sem_post(&ph_full);
                } else {
                    printf("Medida inválida descartada: %.2f\n", measurement);
                }
            }
        }
    }

    // Cerrar el pipe
    close(pipe_fd);
    return NULL;
}

// Función del hilo H-ph
void* ph_thread(void* arg) {
    MonitorArgs* args = (MonitorArgs*)arg;
    FILE* ph_file = fopen(args->ph_file, "a");

    while (1) {
        sem_wait(&ph_full);
        sem_wait(&buffer_mutex);
        fprintf(ph_file, "%.2f (timestamp: %ld)\n", ph_buffer[ph_count - 1].value, ph_buffer[ph_count - 1].timestamp);
        if (ph_buffer[ph_count - 1].value < 6.0 || ph_buffer[ph_count - 1].value > 8.0) {
            printf("Alerta: Valor de pH fuera de rango (%.2f)\n", ph_buffer[ph_count - 1].value);
        }
        sem_post(&buffer_mutex);
        sem_post(&ph_empty);
    }

    fclose(ph_file);
    return NULL;
}

// Función del hilo H-temperatura
void* temperature_thread(void* arg) {
    MonitorArgs* args = (MonitorArgs*)arg;
    FILE* temp_file = fopen(args->temp_file, "a");

    while (1) {
        sem_wait(&temperature_full);
        sem_wait(&buffer_mutex);
        fprintf(temp_file, "%.2f (timestamp: %ld)\n", temperature_buffer[temperature_count - 1].value, temperature_buffer[temperature_count - 1].timestamp);
        if (temperature_buffer[temperature_count - 1].value < 20.0 || temperature_buffer[temperature_count - 1].value > 31.0) {
            printf("Alerta: Valor de temperatura fuera de rango (%.2f)\n", temperature_buffer[temperature_count - 1].value);
        }
        sem_post(&buffer_mutex);
        sem_post(&temperature_empty);
    }

    fclose(temp_file);
    return NULL;
}

// Función principal del monitor
int main(int argc, char* argv[]) {
    // Procesar argumentos
    MonitorArgs args = {0};
    int opt;
    while ((opt = getopt(argc, argv, "b:t:h:p:")) != -1) {
        switch (opt) {
            case 'b':
                args.buffer_size = atoi(optarg);
                break;
            case 't':
                args.temp_file = optarg;
                break;
            case 'h':
                args.ph_file = optarg;
                break;
            case 'p':
                args.pipe_name = optarg;
                break;
            default:
                fprintf(stderr, "Uso: %s -b tam_buffer -t file-temp -h file-ph -p pipe-nominal\n", argv[0]);
                exit(EXIT_FAILURE);
        }
    }

    // Inicializar semáforos
    sem_init(&buffer_mutex, 0, 1);
    sem_init(&temperature_full, 0, 0);
    sem_init(&temperature_empty, 0, args.buffer_size);
    sem_init(&ph_full, 0, 0);
    sem_init(&ph_empty, 0, args.buffer_size);

    // Crear hilos
    pthread_t collector_tid, ph_tid, temperature_tid;
    pthread_create(&collector_tid, NULL, collector_thread, (void*)&args);
    pthread_create(&ph_tid, NULL, ph_thread, (void*)&args);
    pthread_create(&temperature_tid, NULL, temperature_thread, (void*)&args);

    // Esperar a la finalización de los hilos
    pthread_join(collector_tid, NULL);
    pthread_join(ph_tid, NULL);
    pthread_join(temperature_tid, NULL);

    // Destruir semáforos
    sem_destroy(&buffer_mutex);
    sem_destroy(&temperature_full);
    sem_destroy(&temperature_empty);
    sem_destroy(&ph_full);
    sem_destroy(&ph_empty);

    return 0;
}
