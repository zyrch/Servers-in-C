all: server utils utils_header

server: utils utils_header
	gcc server.c -o server utils.o

utils: utils_header
	gcc -c utils.c

utils_header:
	gcc utils.h

clean: 
	rm -f utils.o server utils.h.gch
