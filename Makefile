CC = gcc
CFLAGS = -Wall -Wextra -Werror

all: monitor sensor

monitor: monitor.c metodosMonitor.c
	$(CC) $(CFLAGS) -o monitor metodosMonitor.c monitor.c 

sensor: sensor.c metodosSensor.c
	$(CC) $(CFLAGS) -o sensor metodosSensor.c sensor.c

clean:
	rm -f monitor sensor