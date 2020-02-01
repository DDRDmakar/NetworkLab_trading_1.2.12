
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/poll.h>

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 6000
#define BUFFER_LEN  256
#define TAKETOKEN   pch = strtok(NULL, " ");
#define ADMIN_KEY 3000000

// First 4 bytes - message type
typedef enum MESSAGE_TYPE
{
	MTYPE_RESPONSE_OK = 30,
	
	MTYPE_DISCONNECT,
	MTYPE_TEXT,
	
	MTYPE_ERROR_NAME,
	MTYPE_ERROR_NUM,
	MTYPE_ERROR_RIGHTS,
	MTYPE_ERROR_DISCONNECT,
	MTYPE_ERROR_COMMAND,
	MTYPE_ERROR_PRICE
} MESSAGE_TYPE;

void error_out(const char* err)
{
	printf("\033[0;31m%s\033[0m\n", err);
	exit(1);
}

// READ N BYTES FROM FILE DESCRIPTOR INTO CHAR BUFFER
int custom_read(int desc, char *buf, const unsigned int buf_len)
{
	int n = read(desc, buf, buf_len); // Чтение до buf_len байт из файлового дескриптора
	if (n < 0) error_out("Error reading from socket buffer");
	return n;
}

int main(int argc, char **argv)
{
	if (argc < 4)
	{
		printf("Not enough arguments\n");
		return 1;
	}
	
	int c = socket(AF_INET, SOCK_STREAM, 0);
	if (c < 0) error_out("Socket creation failed");
	
	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	const char *client_name = argv[1];
	const char *client_status_str = argv[2];
	const char *server_addr = argv[3];
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(server_addr);
	serv_addr.sin_port = htons(SERVER_PORT);
	printf("Server socket: %s:%d\n", server_addr, SERVER_PORT);
	
	int connection_status = connect(c, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (connection_status) error_out("Error establishing connection");
	
	char buffer[BUFFER_LEN];
	int *intvec = (int*)buffer;
	bzero(buffer, BUFFER_LEN);
	
	int read_status = 1;
	int write_status = 1;
	
	// Authorization
	intvec[0] = !strcmp(client_status_str, "admin") ? ADMIN_KEY : 0;
	sprintf(buffer+sizeof(int), "%s", client_name);
	write_status = write(c, buffer, BUFFER_LEN);
	if (write_status < 0) error_out("Error writing data");
	
	while (read_status > 0 && strcmp(buffer, "q"))
	{
		bzero(buffer, BUFFER_LEN);
		
		// Read from stdin and socket fd
		struct pollfd pfds[2];
		pfds[0].fd = STDIN_FILENO;
		pfds[0].events = POLLIN;
		pfds[0].revents = 0;
		pfds[1].fd = c;
		pfds[1].events = POLLIN;
		pfds[1].revents = 0;
		
		poll(pfds, 3, -1); // Wait here
		
		// If we typed command in client console
		if (pfds[0].revents & POLLIN)
		{
			printf("Reading stdin\n");
			
			fgets(buffer, BUFFER_LEN, stdin);
			buffer[strlen(buffer) - 1] = '\0'; // Remove \n in the end
			
			if (!strcmp(buffer, "q")) break;
			
			write_status = write(c, buffer, BUFFER_LEN);
			if (write_status < 0) error_out("Error writing data");
		}
		
		// If we have incoming message from server
		if (pfds[1].revents & POLLIN)
		{
			printf("Reading socket\n");
			
			bzero(buffer, BUFFER_LEN);
			
			read_status = custom_read(c, buffer, BUFFER_LEN);
			if (read_status == 0)
			{
				printf("Disconnected from server\n");
				goto close_socket; // If server closed connection, just close socket
			}
			
			MESSAGE_TYPE t = intvec[0];
			printf("Return code: %d\n", t);
			
			switch (t)
			{
				case MTYPE_RESPONSE_OK: { printf("Server response: OK\n"); break; }
				case MTYPE_DISCONNECT:  { printf("Server closed connection\n"); break; }
				case MTYPE_TEXT:        { printf("\033[1;32m%s\033[0m\n", buffer+sizeof(int)); break; }
				
				case MTYPE_ERROR_NAME:       { printf("Error - incorrect name\n"); break; }
				case MTYPE_ERROR_NUM:        { printf("Error - incorrect number passed\n"); break; }
				case MTYPE_ERROR_RIGHTS:     { printf("Error - You are not allowed to do this\n"); break; }
				case MTYPE_ERROR_DISCONNECT: { printf("Error - You cannot disconnect this user\n"); break; }
				case MTYPE_ERROR_COMMAND:    { printf("Error - Incorrect command format\n"); break; }
				case MTYPE_ERROR_PRICE:      { printf("Error - given price is not the highest\n"); break; }
				default:
				{
					printf("Unknown response code\n");
					break;
				}
			}
		}
	}
	
	if (shutdown(c, SHUT_RDWR)) error_out("TCP connection shutdown failed");
	close_socket: ;
	if (close(c)) error_out("Socket closing failed");
	
	return 0;
}
