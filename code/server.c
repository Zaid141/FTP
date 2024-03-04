#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/select.h>

#define CPORT 21

int main()
{
	// initializing with usernames and passwords from users.csv
	char *usernames[256];
	char *passwords[256];
	char *token;
	char row[256];
	int usersize = 0;

	FILE *fptr;
	fptr = fopen("../users.csv", "r");
	int i = 0;
	while (!feof(fptr))
	{
		fgets(row, 255, fptr);

		if (row[strlen(row) - 1] == '\n')
		{
			row[strlen(row) - 1] = '\0';
		}

		token = strtok(row, ",");
		if (token != NULL)
		{
			usernames[i] = strdup(token); //so that usernames[i] isn't just an alias for token but actually the value of token
			usersize++;
		}

		token = strtok(NULL, ",");
		if (token != NULL)
		{
			passwords[i] = strdup(token); 
		}

		//printf("username(%d) = %s , password = %s \n", i, usernames[i], passwords[i]); //debug purposes

		i++;
	}
	fclose(fptr);

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	//printf("Server fd = %d \n", server_socket);

	// check for fail error
	if (server_socket < 0)
	{
		perror("socket:");
		exit(EXIT_FAILURE);
	}

	// setsock
	int value = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); //&(int){1},sizeof(int)

	// define server address structure
	struct sockaddr_in server_address;
	bzero(&server_address, sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(CPORT);
	server_address.sin_addr.s_addr = INADDR_ANY;

	// define client address structure
	struct sockaddr_in client_address;
	socklen_t addr_size = sizeof(client_address);

	// bind
	if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) < 0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	// listen
	if (listen(server_socket, 5) < 0)
	{
		perror("listen failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}

	// DECLARE 2 fd sets (file descriptor sets : a collection of file descriptors)
	fd_set all_sockets;
	fd_set ready_sockets;

	/*In the previous example, we used FD_SETSIZE as the first argument for the select(). (typically 1024)
	Let's say you only have 2 sockets, my sevrer socket and one connected client, each time select() returns, you are
	going to have to check 1024 different possibilitites every time through the loop. If we want to make this more
	efficient, then we can adjust this by keeping track of the largest socket we've seen so far. So that way, if
	my server socket is equal to 3, and I have one client, so that's 4, so this way we're only going to have to go up to
	4 each time (instead of the 1024)
	*/
	int max_socket_so_far = server_socket;

	FD_ZERO(&all_sockets);
	FD_SET(server_socket, &all_sockets);

	//printf("Server is listening...\n");

	while (1)
	{

		ready_sockets = all_sockets;

		// changed 1st argument here
		// 1st argument: range of file descriptors to check  [the highest file descriptor plus one]
		if (select(max_socket_so_far + 1, &ready_sockets, NULL, NULL, NULL) < 0)
		{
			perror("select error");
			exit(EXIT_FAILURE);
		}

		// we changed the range of the for loop as well
		for (int fd = 0; fd <= max_socket_so_far; fd++)
		{
			if (FD_ISSET(fd, &ready_sockets))
			{

				if (fd == server_socket)
				{
					int client_sd = accept(server_socket, (struct sockaddr *)&client_address,
			&addr_size);
					printf("Connection established with user %d \n", client_sd);
					printf("Their port: %d\n\n", ntohs(client_address.sin_port));

					send(client_sd, "220 Service ready for new user.", 32, 0);


					FD_SET(client_sd, &all_sockets);

					// update max_socket_so_far
					if (client_sd > max_socket_so_far)
						max_socket_so_far = client_sd;
				}
				else
				{
					char buffer[256];
					bzero(buffer, sizeof(buffer));
					int bytes = recv(fd, buffer, sizeof(buffer), 0);
					if (bytes == 0) // client has closed the connection
					{
						printf("connection closed from client side \n");

						close(fd);

						FD_CLR(fd, &all_sockets);
					}
					//printf("%s \n", buffer); //debug purposes

					char substring[6]; //to store the commands from user like PASS, RETR etc.
					for (int i = 0; i < 5; i++)
					{
						substring[i] = buffer[i];
					}
					substring[5] = '\0';
					//printf("%s ,\n", substring); //debug

					if (!strncmp("USER", substring,4))
					{
						

						char argument[256]; //to store the arguments from user input
						bzero(argument, sizeof(argument));
						for (int i = 5; i < strlen(buffer); i++)
						{
							argument[i - 5] = buffer[i];
						}
						argument[strlen(buffer)] = '\0';
						// for (int i = 0; i < sizeof(argument); i++) //debug purposes
						// {
						// 	printf("%c", argument[i]);
						// }

						
						int found = -1;
						for (int i = 0; i < usersize; i++)
						{
							if (!strcmp(argument, usernames[i])) //username found
							{
								//printf("found\n");	//debug purposes
								found = i;
								break;
							}
						}

						if (found != -1)
						{
							printf("Successful username verification\n\n");

							send(fd, "331 Username OK, need password.", 32, 0);

							bzero(buffer, sizeof(buffer));
							int bytes = recv(fd, buffer, sizeof(buffer), 0); // get PASS password

							for (int i = 0; i < 5; i++)
							{
								substring[i] = buffer[i];
							}
							if (!strcmp("PASS ", substring))
							{
								bzero(argument, sizeof(argument));
								for (int i = 5; i < sizeof(buffer); i++)
								{
									argument[i - 5] = buffer[i];
								}

								if (!strcmp(argument, passwords[found]))
								{
									printf("Successful login\n\n");

									send(fd, "230 User logged in, proceed.", 29, 0);
								}
								else
								{
									send(fd, "530 Not logged in.", 38, 0); // password content wrong
								}
							}
							else
							{
								send(fd, "530 Not logged in.", 30, 0); // user sent some command other than PASS (maybe can be considered as "503 Bad sequence of commands.")
							}
						}
						else
						{
							send(fd, "530 Not logged in.", 19, 0);  //user not found
						}
					}
					else if (!strcmp("PASS ", substring))
					{
						send(fd, "530 Not logged in.", 19, 0); // sent PASS before USER
					}
					else
					{
						send(fd, "202 Command not implemented.", 29, 0);
					}
				}
			}
		}
	}

	close(server_socket);
	return 0;
}
