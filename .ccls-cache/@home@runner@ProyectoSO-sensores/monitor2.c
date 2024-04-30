#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h> 
#include <errno.h>

#define BUFFERSIZE 128

typedef struct {
    int sensor_type;
    int time_interval;
    char* filename;
    char* pipe_name;
} SensorArgs;

SensorArgs process_arguments(int argc, char* argv[]) {
    SensorArgs args = { .sensor_type = -1, .time_interval = -1, .filename = NULL, .pipe_name = NULL };
    const char* flags[] = {"-s", "-t", "-f", "-p"};
    int num_flags = sizeof(flags) / sizeof(flags[0]);

    for (int i = 1; i < argc; i++) {
        for (int j = 0; j < num_flags; j++) {
            if (strcmp(argv[i], flags[j]) == 0 && i + 1 < argc) {
                switch (j) {
                    case 0: args.sensor_type = atoi(argv[i + 1]); break;
                    case 1: args.time_interval = atoi(argv[i + 1]); break;
                    case 2: args.filename = argv[i + 1]; break;
                    case 3: args.pipe_name = argv[i + 1]; break;
                }
                i++; // Skip next argument as it is part of this flag
                break;
            }
        }
    }
    return args;
}

FILE* open_file(char* filename, char* mode) {
    FILE* file = fopen(filename, mode);
    if (!file) {
        perror("Error opening file");
        exit(1);
    }
    return file;
}

int open_pipe(char* pipe_name, int mode) {
    if (mkfifo(pipe_name, 0666) == -1 && errno != EEXIST) {
        perror("mkfifo");
        exit(1);
    }
    int pipe_fd = open(pipe_name, mode);
    if (pipe_fd < 0) {
        perror("Error opening pipe");
        exit(1);
    }
    return pipe_fd;
}

void send_measurement(int pipe_fd, float measurement) {
    if (write(pipe_fd, &measurement, sizeof(float)) < 0) {
        perror("Error writing to pipe");
        exit(1);
    }
}

int validate_temperature(float measurement) {
    if (measurement >= 20.0 && measurement <= 31.0) {
        return 1;
    } else if (measurement > 0.0) {
        printf("Alert: Temperature out of range (%.2f)\n", measurement);
    } else {
        printf("Negative temperature discarded (%.2f)\n", measurement);
    }
    return 0;
}

int validate_ph(float measurement) {
    if (measurement >= 6.0 && measurement <= 8.0) {
        return 1;
    } else if (measurement > 0.0) {
        printf("Alert: pH value out of range (%.2f)\n", measurement);
    } else {
        printf("Negative pH discarded (%.2f)\n", measurement);
    }
    return 0;
}

void simulate_sensor(int sensor_type, int time_interval, char* filename, char* pipe_name) {
    int pipe_fd = open_pipe(pipe_name, O_WRONLY);
    FILE* file = open_file(filename, "r");
    char buffer[BUFFERSIZE];

    while (fgets(buffer, BUFFERSIZE, file) != NULL) {
        float measurement = atof(buffer);
        int valid_measurement = (sensor_type == 1) ? validate_temperature(measurement) : validate_ph(measurement);
        if (valid_measurement) {
            send_measurement(pipe_fd, measurement);
        }
        sleep(time_interval);
    }
    close(pipe_fd);
    fclose(file);
}

int main(int argc, char* argv[]) {
    SensorArgs args = process_arguments(argc, argv);
    if (args.sensor_type < 0 || args.time_interval < 0 || args.filename == NULL || args.pipe_name == NULL) {
        fprintf(stderr, "Missing arguments. Usage: %s -s <sensor_type> -t <time_interval> -f <filename> -p <pipe_name>\n", argv[0]);
        exit(1);
    }

    simulate_sensor(args.sensor_type, args.time_interval, args.filename, args.pipe_name);
    /*
    pid_t pid = fork();
    if (pid == 0) {
        // Child process
        simulate_sensor(args.sensor_type, args.time_interval, args.filename, args.pipe_name);
        exit(0);
    } else if (pid > 0) {
        // Parent process
        wait(NULL); // Espera a que el proceso hijo termine
    } else {
        perror("fork failed");
        exit(1);
    }*/

    return 0;
}
