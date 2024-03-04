#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>

#define CPORT 21

int main()
{
	// create a socket
	int network_socket;
	network_socket = socket(AF_INET, SOCK_STREAM, 0);

	// check for fail error
	if (network_socket == -1)
	{
		printf("socket creation failed..\n");
		exit(EXIT_FAILURE);
	}

	// setsock
	int value = 1;
	setsockopt(network_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); //&(int){1},sizeof(int)

	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(CPORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	char buffer[256];

	// connect
	if (connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}
	else
	{
		printf("Hello!! Please Authenticate to run server commands\n1. type \"USER\" followed by a space and your username\n2. type \"PASS\" followed by a space and your password\n\n\"QUIT\" to close connection at any moment\nOnce Authenticated\n this is the list of commands :\n\"STOR\" + space + filename |to send a file to the server\n\"RETR\" + space + filename |to download a file from the server\n\"LIST\" to to list all the files under the current server directory\n\"CWD\" + space + directory |to change the current server directory\n\"PWD\" to display the current server directory\nAdd \"!\" before the last three commands to apply them locally\n\n");
		bzero(buffer, sizeof(buffer));
		int bytes = recv(network_socket, buffer, sizeof(buffer), 0);
		printf("%s\n", buffer);

		bzero(buffer, sizeof(buffer));
	}

	while (1)
	{
		// get input from user
		fgets(buffer, sizeof(buffer), stdin);
		buffer[strcspn(buffer, "\n")] = 0; // remove trailing newline char from buffer, fgets does not remove it
		if (strcmp(buffer, "exit") == 0)
		{
			printf("closing the connection to server \n");
			close(network_socket);
			break;
		}

		if (send(network_socket, buffer, strlen(buffer), 0) < 0)
		{
			perror("send");
			exit(EXIT_FAILURE);
		}

		bzero(buffer, sizeof(buffer));
		int bytes = recv(network_socket, buffer, sizeof(buffer), 0);
		printf("%s\n", buffer);
	}

	return 0;
}
