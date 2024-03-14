#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <unistd.h>
#include <stdlib.h>

#include <sys/stat.h>

#define CPORT 21

int portcmd(int network_socket, char *address, int port, char *args, int addition)
{

	// int addition = 1;
	char buffer[256];

	char *h1;
	char *h2;
	char *h3;
	char *h4;

	int newport = port + addition;
	int p1 = newport / 256;
	int p2 = newport % 256;

	char *token = strtok(address, ".");
	if (token == NULL)
	{
		fprintf(stderr, "Invalid address format\n");
		return 1;
	}
	h1 = token;

	token = strtok(NULL, ".");
	if (token == NULL)
	{
		fprintf(stderr, "Invalid address format\n");
		return 1;
	}
	h2 = token;

	token = strtok(NULL, ".");
	if (token == NULL)
	{
		fprintf(stderr, "Invalid address format\n");
		return 1;
	}
	h3 = token;

	token = strtok(NULL, ".");
	if (token == NULL)
	{
		fprintf(stderr, "Invalid address format\n");
		return 1;
	}
	h4 = token;

	// sprintf(p1, "%d", port/256);
	// sprintf(p2, "%d", port%256);

	char command[256];
	sprintf(command, "PORT %s,%s,%s,%s,%d,%d", h1, h2, h3, h4, p1, p2);
	// sprintf(command, "PORT %s.%s.%s.%s:%d", h1, h2, h3, h4, newport);

	if (send(network_socket, command, strlen(command), 0) < 0)
	{
		perror("send");
		return 1;
	}

	recv(network_socket, buffer, 19, 0);

	if (strncmp(buffer, "530 Not logged in.", 19)==0)
	{
		printf("%s\n", buffer);
		return 0;
	}
	

	int data_socket;
	data_socket = socket(AF_INET, SOCK_STREAM, 0);

	// check for fail error
	if (data_socket == -1)
	{
		printf("socket creation failed..\n");
		exit(EXIT_FAILURE);
	}

	// setsock
	// int value = 1;
	// setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); //&(int){1},sizeof(int)

	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(20); // port 20
	server_address.sin_addr.s_addr = INADDR_ANY;

	struct sockaddr_in client_address;
	socklen_t len = sizeof(client_address);
	client_address.sin_family = AF_INET;
	client_address.sin_port = htons(newport); // port = newport
	client_address.sin_addr.s_addr = INADDR_ANY;

	int value = 1;
	setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

	if (bind(data_socket, (struct sockaddr *)&client_address, sizeof(struct sockaddr_in)) == 0)
		printf("Binded Correctly\n");
	else
		printf("Unable to bind\n");

	// connect
	if (connect(data_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}

	
	bzero(buffer, sizeof(buffer));
	int bytes = recv(network_socket, buffer, 28, 0); // used to be data_socket instead of network_socket
	if (bytes == 0)
	{
		printf("zero bytes received: %s\n", buffer);
	}
	else
	{
		printf("PORT:: %s\n", buffer); // port command successful
	}

	// send(data_socket, buffer, sizeof(buffer),0);

	bzero(buffer, sizeof(buffer));
	bytes = recv(network_socket, buffer, 52, 0); // used to be data_socket instead of network_socket
	if (bytes == 0)
	{
		printf("zero bytes received: %s\n", buffer);
	}
	else
	{
		printf("FILESTATUS:: %s\n", buffer); // 150 file status okay
	}

	if (strncmp(args, "RETR ", 5) == 0)
	{
		send(data_socket, args, 256, 0);
		char *token = strtok(args, " ");
		if (token == NULL)
		{
			fprintf(stderr, "Invalid address format\n");
			close(data_socket);
			return 1;
		}
		char *arg1 = token;

		token = strtok(NULL, " ");
		if (token == NULL)
		{
			fprintf(stderr, "Invalid address format\n");
			close(data_socket);
			return 1;
		}
		char *filename = token;

		int n = 1;
		bzero(buffer, sizeof(buffer));
		recv(data_socket, buffer, sizeof(buffer), 0); // receive file exists or not (on server's side)
		if (strncmp(buffer, "550 No such file or directory.", 30) == 0)
		{
			//close(server_socket);
			close(data_socket);
			printf("550 No such file or directory.\n"); // debugging
			return 0;
		}
		else
		{
			printf("%s\n", buffer); // debugging
		}

		FILE *fptr2 = fopen(filename, "w");
		if (fptr2 == NULL)
		{
			perror("[-]Error in creating file.");
			close(data_socket);
			exit(1);
		}

		// int size = 0;
		// recv(data_socket, &size, sizeof(size), 0); // receive size
		// printf("size: %d\n", size);				   // debugging

		while (n > 0)
		{
			bzero(buffer, sizeof(buffer));
			n = recv(data_socket, buffer, sizeof(buffer), 0);

			printf("%s\n", buffer); // debugging
			fwrite(buffer, 1, n, fptr2);
		}

		fclose(fptr2);
		close(data_socket);

		bzero(buffer, sizeof(buffer));
		recv(network_socket, buffer, sizeof(buffer), 0);
		printf("transfer: %s\n", buffer); // 226 transfer completed
	}
	else if (strncmp(args, "STOR ", 5) == 0)
	{
		send(data_socket, args, 256, 0);
		char *token = strtok(args, " ");
		if (token == NULL)
		{
			fprintf(stderr, "Invalid address format\n");
			close(data_socket);
			return 1;
		}
		char *arg1 = token;

		token = strtok(NULL, " ");
		if (token == NULL)
		{
			fprintf(stderr, "Invalid address format\n");
			close(data_socket);
			return 1;
		}
		char *filename = token;

		FILE *fptr;
		fptr = fopen(filename, "rb");
		if (fptr == NULL)
		{
			printf("550 No such file or directory.\n"); // debugging
			send(data_socket, "550 N", 6, 0);
			close(data_socket);
			fclose(fptr);
			return 0;
		}
		else
		{
			send(data_socket, "found", 6, 0);
		}
		

		struct stat st;
		stat(filename, &st);
		int size = st.st_size;

		printf("size: %d\n", size); // debugging

		if (send(data_socket, &size, sizeof(size), 0) == -1) // sending file size
		{
			perror("[-] Error in sending data");
			exit(1);
		}

		char data[256];

		int n;
		int readsofar = 0;
		while (readsofar != size)
		{
			bzero(data, sizeof(data));
			// printf("ftell: %ld\n", ftell(fptr)); // debugging
			n = fread(data, 1, sizeof(data), fptr);

			if (n <= 0)
			{
				if (feof(fptr))
				{
					// End of file reached
					break;
				}
				else
				{
					perror("Error reading from file");
					close(data_socket);
					exit(1);
				}
			}
			readsofar += n;
			// printf("readsofar: %d\n", readsofar);	   // debugging
			if (send(data_socket, data, n, 0) == -1) // replace sizeofdata with n
			{
				perror("[-] Error in sending data");
				close(data_socket);
				exit(1);
			}
			// printf("ftell2: %ld\n", ftell(fptr)); //debugging
			// printf("Content: %s\n", data); // debugging

			// printf("ftell2: %ld\n", ftell(fptr)); // debugging
		}
		fclose(fptr);
		close(data_socket);
		bzero(buffer, sizeof(buffer));
		n = recv(network_socket, buffer, sizeof(buffer), 0);
		printf("%s\n", buffer); // 226 transfer completed
	}
	else if (strncmp(args, "LIST", 4) == 0)
	{
		send(data_socket, args, 256, 0);
		int n = 1;
		while (n > 0)
		{
			bzero(buffer, sizeof(buffer));
			n = recv(data_socket, buffer, sizeof(buffer), 0);

			printf("%s\n", buffer); //print directories
			
		}
		close(data_socket);
		bzero(buffer, sizeof(buffer));
		n = recv(network_socket, buffer, sizeof(buffer), 0);
		printf("%s\n", buffer); // 226 transfer completed

	}

	return 0;
}

int main()
{
	int addition = 1;
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

	struct sockaddr_in client;
	socklen_t len = sizeof(client);

	// connect
	if (connect(network_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("connect");
		exit(EXIT_FAILURE);
	}
	else
	{

		if (getsockname(network_socket, (struct sockaddr *)&client, &len) == -1)
		{
			perror("getsockname");
		}
		// else		//debugging
		// {
		// 	printf("address number %s\n", inet_ntoa(client.sin_addr));
		// 	printf("port number %d\n", ntohs(client.sin_port));
		// }

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
		printf("buffer: %s\n", buffer);	   // debugging
		if (strcmp(buffer, "exit") == 0)
		{
			printf("closing the connection to server \n");
			close(network_socket);
			break;
		}
		else if (strncmp(buffer, "RETR ", 5) == 0 || strncmp(buffer, "STOR ", 5) == 0 || strncmp(buffer, "LIST", 4) == 0)
		{
			printf("in retr else if\n"); // debugging
			if (portcmd(network_socket, inet_ntoa(client.sin_addr), ntohs(client.sin_port), buffer, addition))
			{
				perror("portcmd fail");
				exit(EXIT_FAILURE);
			}
			addition++;
		}
		else
		{
			printf("in final else\n"); // debugging
			if (send(network_socket, buffer, strlen(buffer), 0) < 0)
			{
				perror("send");
				exit(EXIT_FAILURE);
			}

			bzero(buffer, sizeof(buffer));
			int bytes = recv(network_socket, buffer, sizeof(buffer), 0);
			printf("%s\n", buffer);
		}
	}

	return 0;
}
