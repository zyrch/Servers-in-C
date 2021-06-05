.DEFAULT_GOAL := sequential
CC=gcc
PORT = 9090
sequential: 
	${CC} -Wall server_copy.cpp utils.c -o rheagal-core.out
	./rheagal-core.out
clean:
	@echo "Cleaning up..."
	rm *.out
test: 
	python3 sample-client.py localhost ${PORT}
