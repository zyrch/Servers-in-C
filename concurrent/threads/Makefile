all: server utils utils_header

server: utils utils_header
	gcc server.c -o server utils.o -pthread

utils: utils_header
	gcc -c ../../utils/utils.c

utils_header:
	gcc ../../utils/utils.h

clean: 
	rm -f utils.o server ../../utils/utils.h.gch
