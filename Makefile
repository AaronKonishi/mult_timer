
all: mt_timer.c test.c
	gcc test.c mt_timer.c -pthread -o timer -Wall

clean:
	rm timer
