
/*
 * 
 * Разработка простейшего клиента TCP
 * 
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_ADDR "127.0.0.1"
#define SERVER_PORT 6000
#define BUFFER_LEN  256

// First 4 bytes - message type
enum MESSAGE_TYPE
{
	MTYPE_LIST,
	MTYPE_PRICE,
	MTYPE_ADD,
	MTYPE_FINISH
};

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
	int c = socket(AF_INET, SOCK_STREAM, 0);
	if (c < 0) error_out("Socket creation failed");
	
	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(SERVER_ADDR);
	serv_addr.sin_port = htons(SERVER_PORT);
	printf("Server socket: %s:%d\n", SERVER_ADDR, SERVER_PORT);
	
	int connection_status = connect(c, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
	if (connection_status) error_out("Error establishing connection");
	
	char buffer[BUFFER_LEN];
	int *intvec = (int*)buffer;
	bzero(buffer, BUFFER_LEN);
	
	int read_status = 1;
	char *cl[10];
	
	while (read_status > 0 && strcmp(buffer, "q"))
	{
		bzero(buffer, BUFFER_LEN);
		fgets(buffer, BUFFER_LEN, stdin);
		buffer[strlen(buffer) - 1] = '\0'; // Remove \n in the end
		
		char pch = strtok (buffer, "");//////////////////////
		while (pch != NULL)
		{
			printf ("%s\n",pch);
			pch = strtok (NULL, " ,.-");
		}
		
		if (!strcmp(buffer, "list"))
		{
			intvec[0] = MTYPE_LIST;
		}
		if (!strcmp(buffer, "add"))
		{
			intvec[0] = MTYPE_ADD;
			intvec[1] = 100;
			sprintf(buffer+(2*sizeof(int)), "%s", "Nameeee");
		}
		
		int write_status = write(c, buffer, BUFFER_LEN);
		if (write_status < 0) error_out("Error writing data");
		
		bzero(buffer, BUFFER_LEN);
		
		read_status = custom_read(c, buffer, BUFFER_LEN);
		
		if (read_status == 0)
		{
			printf("Disconnected from server\n");
			goto close_socket; // If server closed connection, just close socket
		}
		
		printf("\033[1;32m[%s]\033[0m\n", buffer);
	}
	
	if (shutdown(c, SHUT_RDWR)) error_out("TCP connection shutdown failed");
	close_socket: ;
	if (close(c)) error_out("Socket closing failed");
	
	return 0;
}
