
all: timer.c test.c
	gcc test.c timer.c -pthread -o timer -Wall

clean:
	rm timer
