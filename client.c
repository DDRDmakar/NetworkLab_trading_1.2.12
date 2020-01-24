
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
#define TAKETOKEN   pch = strtok(NULL, " ");
#define ADMIN_KEY 3000000

// First 4 bytes - message type
enum MESSAGE_TYPE
{
	MTYPE_RESPONSE_OK,
	
	MTYPE_LIST,
	MTYPE_PRICE,
	MTYPE_ADD,
	MTYPE_FINISH,
	
	MTYPE_ERROR_NAME
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
	int write_status = 1;
	
	// Authorization
	intvec[0] = ADMIN_KEY;
	sprintf(buffer+sizeof(int), "%s", "FOMA");
	write_status = write(c, buffer, BUFFER_LEN);
	if (write_status < 0) error_out("Error writing data");
	
	while (read_status > 0 && strcmp(buffer, "q"))
	{
		bzero(buffer, BUFFER_LEN);
		fgets(buffer, BUFFER_LEN, stdin);
		buffer[strlen(buffer) - 1] = '\0'; // Remove \n in the end
		
		char *pch = strtok(buffer, " ");
		/*while (pch != NULL)
		{
			printf ("token - %s\n",pch);
			pch = strtok(NULL, " ");
		}*/
		
		// LIST
		if (!strcmp(pch, "list"))
		{
			intvec[0] = MTYPE_LIST;
		}
		// ADD
		else if (!strcmp(pch, "add"))
		{
			intvec[0] = MTYPE_ADD; // Add
			TAKETOKEN;
			intvec[1] = atoi(pch); // Price
			TAKETOKEN;
			sprintf(buffer+(2*sizeof(int)), "%s", pch); // Name
		}
		// PRICE
		else if (!strcmp(pch, "price"))
		{
			intvec[0] = MTYPE_PRICE;
			TAKETOKEN;
			intvec[1] = atoi(pch);
			TAKETOKEN;
			sprintf(buffer+(2*sizeof(int)), "%s", pch);
		}
		
		write_status = write(c, buffer, BUFFER_LEN);
		if (write_status < 0) error_out("Error writing data");
		
		bzero(buffer, BUFFER_LEN);
		
		read_status = custom_read(c, buffer, BUFFER_LEN);
		
		if (read_status == 0)
		{
			printf("Disconnected from server\n");
			goto close_socket; // If server closed connection, just close socket
		}
		
		if (*((int*)buffer) == MTYPE_RESPONSE_OK)
		{
			printf("\033[1;32mServer response: OK\033[0m\n");
		}
		else printf("\033[1;32m[%s]\033[0m\n", buffer);
	}
	
	if (shutdown(c, SHUT_RDWR)) error_out("TCP connection shutdown failed");
	close_socket: ;
	if (close(c)) error_out("Socket closing failed");
	
	return 0;
}
