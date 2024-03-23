all: client/client server/server

client/client: ./code/client.c
	gcc code/client.c -o client/client

server/server: ./code/server.c
	gcc code/server.c -o server/server

clean:
	rm -rf client/client server/server *.