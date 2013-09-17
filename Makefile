CC = gcc
DEBUGFLAGS = -g -Wall -I
CFLAGS = -D_REENTRANT $(DEBUGFLAGS) -D_XOPEN_SOURCE=500
LDFLAGS = -pthread

all: snowcast_listener snowcast_control snowcast_server
snowcast_server:   snowcast_server.c serial.c
snowcast_listener: snowcast_listener.c serial.c
snowcast_control:  snowcast_control.c serial.c
 
clean:
	rm -f snowcast_listener snowcast_control snowcast_server
