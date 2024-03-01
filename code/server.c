#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/time.h>
#include <sys/select.h>

#define PORT 9007


int main()
{
	int server_socket = socket(AF_INET,SOCK_STREAM,0);
	printf("Server fd = %d \n",server_socket);
	
	//check for fail error
	if(server_socket<0)
	{
		perror("socket:");
		exit(EXIT_FAILURE);
	}

	//setsock
	int value  = 1;
	setsockopt(server_socket,SOL_SOCKET,SO_REUSEADDR,&value,sizeof(value)); //&(int){1},sizeof(int)
	
	//define server address structure
	struct sockaddr_in server_address;
	bzero(&server_address,sizeof(server_address));
	server_address.sin_family = AF_INET;
	server_address.sin_port = htons(PORT);
	server_address.sin_addr.s_addr = INADDR_ANY;


	//bind
	if(bind(server_socket, (struct sockaddr*)&server_address,sizeof(server_address))<0)
	{
		perror("bind failed");
		exit(EXIT_FAILURE);
	}

	//listen
	if(listen(server_socket,5)<0)
	{
		perror("listen failed");
		close(server_socket);
		exit(EXIT_FAILURE);
	}
	

	//DECLARE 2 fd sets (file descriptor sets : a collection of file descriptors)
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
	FD_SET(server_socket,&all_sockets);


	printf("Server is listening...\n");

	while(1)
	{		
		
		ready_sockets = all_sockets;


		//changed 1st argument here
		//1st argument: range of file descriptors to check  [the highest file descriptor plus one]
		if(select(max_socket_so_far+1,&ready_sockets,NULL,NULL,NULL)<0)
		{
			perror("select error");
			exit(EXIT_FAILURE);
		}

		
		//we changed the range of the for loop as well
		for(int fd = 0 ; fd <= max_socket_so_far; fd++)
		{
			if(FD_ISSET(fd,&ready_sockets))
			{
			
				if(fd==server_socket)
				{
					int client_sd = accept(server_socket,0,0);
					printf("Client Connected fd = %d \n",client_sd);
					
					FD_SET(client_sd,&all_sockets);

					//update max_socket_so_far
					if(client_sd > max_socket_so_far)	
						max_socket_so_far = client_sd;
					
				}

				else
				{
					char buffer[256];
					bzero(buffer,sizeof(buffer));
					int bytes = recv(fd,buffer,sizeof(buffer),0);
					if(bytes==0)   //client has closed the connection
					{
						printf("connection closed from client side \n");
						
						close(fd);

						FD_CLR(fd,&all_sockets);
						
					}
					printf("%s \n",buffer);
				}
			}
		}

	}

	close(server_socket);
	return 0;
}
