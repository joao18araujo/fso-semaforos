RANDINT:=$(shell python -c 'from random import randint; print(randint(1024, 4096));')

all: 
	gcc -o sem_project -ansi -pedantic -W -Wall -pthread -std=c11 sem_project.c

run:
	./sem_project

clean:
	rm sem_project
