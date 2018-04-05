build: client server

client: client.c
	gcc -Wall -lnsl -g client.c -o client 

server: server.c
	gcc -Wall -lnsl -g server.c -o server 

clean:
	rm -f *.o
