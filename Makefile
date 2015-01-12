C=$(CROSS_COMPILE)gcc -Wall -g
CPP=$(CROSS_COMPILE)g++ -Wall -g

GFLAGS=`pkg-config --libs --cflags gtk+-2.0 gmodule-2.0`
OFLAGS=`pkg-config --libs --cflags opencv`

all: libsemafor.o main

main: main.cpp 
	$(CPP) main.cpp -lbot -lrt -lpthread -lserial -lgpio libsemafor.o -o main  $(OFLAGS)

libsemafor.o:semafor.c semafor.h
	$(C) -c semafor.c -o libsemafor.o


