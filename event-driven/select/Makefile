all: server utils utils_header

server: utils utils_header
	gcc server.c -o server utils.o

utils: utils_header
	gcc -c ../../src/utils.c

utils_header:
	gcc ../../headers/utils.h

clean: 
	rm -f utils.o server ../../headers/utils.h.gch
