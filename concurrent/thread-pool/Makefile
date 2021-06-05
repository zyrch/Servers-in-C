all: server utils utils_header

server: utils utils_header thpool thpool_header
	gcc server.c -o server utils.o thpool.o -pthread

utils: utils_header
	gcc -c ../../src/utils.c

utils_header:
	gcc ../../headers/utils.h

thpool: thpool_header
	gcc -c ../../src/thpool.c

thpool_header:
	gcc ../../headers/thpool.h

clean: 
	rm -f utils.o thpool.o server ../../headers/utils.h.gch ../../headers/thpool.h.gch
