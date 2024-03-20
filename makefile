all: client/client2 server/server2

client/client2: ./code/client2.c
	gcc code/client2.c -o client/client2

server/server2: ./code/server2.c
	gcc code/server2.c -o server/server2

clean:
	rm -rf client/client2 server/server2 *.