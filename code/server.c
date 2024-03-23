#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/select.h>
#include <errno.h>

#include <sys/stat.h>
#include <dirent.h>

#define CPORT 21

int main()
{
#define MAX_PATH 1024
	char *user_filepaths[FD_SETSIZE];

	int auth[260]; // determines whether a user at fd = i is authenticated or not
	for (int i = 0; i < sizeof(auth); i++)
	{
		auth[i] = 0;
	}

	// initializing with usernames and passwords from users.csv
	char *usernames[256];
	char *passwords[256];
	char *token;
	char row[256];
	int usersize = 0;

	char *currentUsers[256]; 

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
			usernames[i] = strdup(token); // so that usernames[i] isn't just an alias for token but actually the value of token
			usersize++;
		}

		token = strtok(NULL, ",");
		if (token != NULL)
		{
			passwords[i] = strdup(token);
		}

		

		i++;
	}
	fclose(fptr);

	int server_socket = socket(AF_INET, SOCK_STREAM, 0);

	// check for fail error
	if (server_socket < 0)
	{
		perror("socket:");
		exit(EXIT_FAILURE);
	}

	// setsock
	int value = 1;
	setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));

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

	//socket for data connections
	int data_socket;
	data_socket = socket(AF_INET, SOCK_STREAM, 0);

	// check for fail error
	if (data_socket == -1)
	{
		printf("socket creation failed..\n");
		exit(EXIT_FAILURE);
	}

	
	setsockopt(data_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value)); 

	// define data server address structure
	struct sockaddr_in data_address;
	bzero(&data_address, sizeof(data_address));
	data_address.sin_family = AF_INET;
	data_address.sin_port = htons(20); // on port 20
	data_address.sin_addr.s_addr = INADDR_ANY;

	if (bind(data_socket,
			 (struct sockaddr *)&data_address,
			 sizeof(data_address)) < 0)
	{
		printf("socket bind failed..\n");
		printf("Error code: %d\n", errno);
		exit(EXIT_FAILURE);
	}

	// after it is bound, we can listen for connections with a queue length of 5
	if (listen(data_socket, 5) < 0)
	{
		printf("Listen failed..\n");
		close(data_socket);
		exit(EXIT_FAILURE);
	}

	// DECLARE 2 fd sets (file descriptor sets : a collection of file descriptors)
	fd_set all_sockets;
	fd_set ready_sockets;

	
	int max_socket_so_far = server_socket; 

	FD_ZERO(&all_sockets);
	FD_SET(server_socket, &all_sockets); 

	while (1)
	{

		ready_sockets = all_sockets;

		if (select(max_socket_so_far + 1, &ready_sockets, NULL, NULL, NULL) < 0)
		{
			perror("select error");
			exit(EXIT_FAILURE);
		}

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
					user_filepaths[client_sd] = strdup("../server"); // Or another appropriate default directory //ZAID

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
						printf("Closed! \n");
						auth[fd] = 0;
						close(fd);

						FD_CLR(fd, &all_sockets);
					}
					

					char substring[6]; // to store the commands from user like PASS, RETR etc.
					for (int i = 0; i < 5; i++)
					{
						substring[i] = buffer[i];
					}
					substring[5] = '\0';

					if (!strncmp("USER", substring, 4))
					{

						char argument[256]; // to store the arguments from user input
						bzero(argument, sizeof(argument));
						for (int i = 5; i < strlen(buffer); i++)
						{
							argument[i - 5] = buffer[i];
						}
						argument[strlen(buffer)] = '\0';
						

						int found = -1;
						for (int i = 0; i < usersize; i++)
						{
							if (!strcmp(argument, usernames[i])) // username found
							{
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
								for (int i = 5; i < sizeof(buffer); i++) //start from 5 as the argument starts from the 5th character (start from 0)
								{
									argument[i - 5] = buffer[i]; 
								}

								if (!strcmp(argument, passwords[found]))
								{
									printf("Successful login\n\n");
									auth[fd] = 1;								 // client is now authorized
									currentUsers[fd] = strdup(usernames[found]); // Zaid
									// zaid
									//  Calculate the new size needed for the filepath
									//  int newSize = snprintf(NULL, 0, "/server/%s", currentUsers[fd]) + 1; // +1 for the null terminator

									// Allocate new memory for filepath
									// char *newFilepath = (char *)malloc(newSize);

									// if (newFilepath == NULL) {
									// 	fprintf(stderr, "Memory allocation failed\n");
									// 	exit(EXIT_FAILURE);
									// }

									// Format the new filepath with the username
									// snprintf(newFilepath, newSize, "/server/%s", currentUsers[fd]);

									// Since filepath was previously allocated with strdup, free it before reassigning
									// free(filepath);

									// Reassign filepath to point to the new string
									// filepath = newFilepath;

									char cwd[1024];
									if (getcwd(cwd, sizeof(cwd)) != NULL)
									{
										char userDir[1024];
										// Concatenate the cwd, the relative path to server directory, and the username
										// snprintf(userDir, sizeof(userDir), "%s/../server/%s", cwd, currentUsers[fd]);
										// user_filepaths[fd] = userDir;
										char *newPath = (char *)malloc(MAX_PATH);
										if (newPath)
										{
											snprintf(newPath, MAX_PATH, "%s/../server/%s", cwd, currentUsers[fd]);
											user_filepaths[fd] = newPath; // Now you can use filepath
										}
									}

									send(fd, "230 User logged in, proceed.", 29, 0);
								}
								else
								{
									auth[fd] = 0;						   // client is not authorized
									send(fd, "530 Not logged in.", 38, 0); // password content wrong
									printf("Incorrect password\n");
								}
							}
							else
							{
								auth[fd] = 0;						   // client is not authorized
								send(fd, "530 Not logged in.", 30, 0); // user sent some command other than PASS
							}
						}
						else
						{
							auth[fd] = 0;						   // client is not authorized
							send(fd, "530 Not logged in.", 19, 0); // user not found
						}
					}

					else if (strncmp("CWD", buffer, 3) == 0)
					{
						// Code to change directory, now updates user_filepaths[fd]
						char dirPath[1024];
						sscanf(buffer + 4, "%s", dirPath);
						// Allocate new memory with enough space for the updated path
						char *newPath = malloc(MAX_PATH);
						if (newPath)
						{
							snprintf(newPath, MAX_PATH, "%s/%s", user_filepaths[fd], dirPath);
							free(user_filepaths[fd]);	  // Free the old path
							user_filepaths[fd] = newPath; // Update to the new path

							// Send success response...
							char response[2048]; // Ensure the buffer is large enough for the response
							snprintf(response, sizeof(response), "200 directory changed to %s", user_filepaths[fd]);
							send(fd, response, strlen(response), 0);
						}
					}
					else if (strncmp("PWD", buffer, 3) == 0)
					{
						if (auth[fd])
						{ // Ensure the client is authenticated
							if (currentUsers[fd] != NULL)
							{
								char response[3000];
								snprintf(response, sizeof(response), "257 \"%s\" is the current directory.", user_filepaths[fd]);
								
								
								send(fd, response, strlen(response), 0);
							}
							else
							{
								send(fd, "550 Could not retrieve user directory.", 38, 0);
							}
						}
						else
						{
							send(fd, "530 Not logged in.", 19, 0);
						}
					}

					else if (!strcmp("PASS ", substring))
					{
						auth[fd] = 0;						   // client is not authorized
						send(fd, "530 Not logged in.", 19, 0); // sent PASS before USER
					}
					else if (strncmp("QUIT", buffer, 4) == 0)
					{
						// Always allow QUIT, whether authenticated or not
						send(fd, "221 Service closing control connection.", 40, 0);
						close(fd); // Close the client socket

						if (currentUsers[fd] != NULL)
						{
							currentUsers[fd] = NULL;
						}

						if (user_filepaths[fd] != NULL)
						{
							user_filepaths[fd] = NULL;
						}

						FD_CLR(fd, &all_sockets); // Remove from the master set
						auth[fd] = 0;			  // Mark as not authenticated
						printf("Closed! \n");
					}

					else if (!strncmp("PORT", substring, 4)) //sent before handling RETR, STOR, LIST
					{
						if (auth[fd])
						{
							send(fd, "230 User logged in", 19, 0);
							char argument[256]; // to store the arguments from user input
							bzero(argument, sizeof(argument));
							for (int i = 5; i < strlen(buffer); i++)
							{
								argument[i - 5] = buffer[i];
							}
							argument[strlen(buffer)] = '\0';

							printf("Port received: %s\n\n", argument);

							printf("File okay, beginning data connections\n\n");

							send(fd, "200 PORT command successful", 28, 0);

							send(fd, "150 File status okay; about to open data connection", 52, 0);

							printf("Connecting to Client Transfer Socket... \n");

							int client_socket = accept(
								data_socket, (struct sockaddr *)&client_address,
								&addr_size);

							printf("Connection Successful\n");

							pid_t pid = fork();
							if (pid < 0)
							{
								perror("fork failed");
								exit(EXIT_FAILURE);
							}
							else if (pid == 0) // Child process
							{
								// Handle data connection here
								bzero(buffer, sizeof(buffer));
								int bytes = recv(client_socket, buffer, sizeof(buffer), 0); // receive actual command after port
								if (bytes == 0)												// client has closed the connection
								{
									printf("Closed! \n");
									auth[fd] = 0;
									close(fd);

									FD_CLR(fd, &all_sockets);
								}

								token = strtok(buffer, " ");
								if (token == NULL)
								{
									fprintf(stderr, "Invalid address format\n");
									return 1;
								}
								char *arg1 = token;
								char *filename;


								if (strncmp("STOR", arg1, 5) == 0 || strncmp("RETR", arg1, 5) == 0)
								{
									
									token = strtok(NULL, " ");
									if (token == NULL)
									{
										fprintf(stderr, "Invalid address format\n");
										return 1;
									}
									filename = token;
									char openfile[256];
									sprintf(openfile, "%s/%s", user_filepaths[fd], filename);

									if (strncmp("STOR", arg1, 5) == 0)
									{

										int n = 1;

										bzero(buffer, sizeof(buffer));
										recv(client_socket, buffer, 6, 0); // receive file exists or not (on client's side)
										if (strncmp(buffer, "550 No such file or directory.", 5) == 0)
										{
											close(server_socket);
											close(client_socket);
											printf("Issue with file\n"); 
											exit(1);
										}
										

										FILE *fptr = fopen(openfile, "w");
										if (fptr == NULL)
										{
											perror("[-]Error in creating file.");
											exit(1);
										}

										int size = 0;
										recv(client_socket, &size, sizeof(size), 0); // receive size
										

										while (n > 0)
										{
											bzero(buffer, sizeof(buffer));
											n = recv(client_socket, buffer, sizeof(buffer), 0);

											
											fwrite(buffer, 1, n, fptr);
										}

										fclose(fptr);
										close(client_socket);
									}
									else // handling RETR
									{
										
										FILE *fptr;
										fptr = fopen(openfile, "rb");
										if (fptr == NULL)
										{
											send(client_socket, "550 No such file or directory.", 31, 0);
											close(server_socket);
											close(client_socket);
											fclose(fptr);
											printf("Issue with file\n"); 
											exit(1);
										}
										else
										{
											send(client_socket, "found", 6, 0);
										}

										struct stat st;
										stat(openfile, &st);
										int size = st.st_size;

										char data[256];

										int n;
										int readsofar = 0;
										while (readsofar != size)
										{
											bzero(data, sizeof(data));
											
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
													exit(1);
												}
											}
											readsofar += n;
											
											if (send(client_socket, data, n, 0) == -1) 
											{
												perror("[-] Error in sending data");
												exit(1);
											}
											
										}
										fclose(fptr);
										close(client_socket);

										
									}
									
									if (send(fd, "226 Transfer complete.", 24, 0) == -1) //send 226 to client
									{
										perror("[-] Error in sending data");
										exit(1);
									}
									else
									{
										printf("226 Transfer complete.\n"); //print 226 locally
									}
								}
								else if (strncmp("LIST", arg1, 5) == 0)
								{
									printf("Listing directory\n");
									
									DIR *d;
									struct dirent *dir;
									d = opendir(user_filepaths[fd]);
									if (d)
									{
										
										dir = readdir(d); // to purge the '.' dir
										dir = readdir(d); // to purge the '..' dir
										while ((dir = readdir(d)) != NULL)
										{
											
											if (send(client_socket, dir->d_name, 256, 0) == -1)
											{
												perror("[-] Error in sending data");
												close(client_socket);
												exit(1);
											}
										}
										closedir(d);
										close(client_socket);
									}
									if (send(fd, "226 Transfer completed.", 24, 0) == -1) 
									{
										perror("[-] Error in sending data");
										exit(1);
									}
									else
									{
										printf("226 Transfer complete.\n");
									}
									return (0);
								}

								// Close unnecessary sockets
								close(server_socket);
					

								// Terminate the child process
								exit(EXIT_SUCCESS);
							}
							else // Parent process
							{
								// Close the client socket (that is handling data connection) in the parent process
								close(client_socket);
							}
						}
						else
						{
							send(fd, "530 Not logged in.", 19, 0);
						}
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
