all:
	gcc -o worker worker.c
	gcc -o server server.c -lpthread
	gcc -o client client.c

clean:
	rm -f worker server client