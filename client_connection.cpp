
#include <iostream>
#include <unistd.h>
#include <pthread.h>
#include <string>
#include <string.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "server.hpp"
#include "client_connection.hpp"
#include "tools.hpp"


Client_connection::Client_connection(Server *server, const int socket_fd, const sockaddr_in addr, const CLIENT_STATUS status, const std::string id) :
	server (server),
	socket_fd (socket_fd),
	addr (addr),
	status (status),
	id (id)
{
	printf("\033[1;32mClient \"%s\" constructor\033[0m\n", id.c_str());
	
	// init mutex
	pthread_mutexattr_init(&matr_client_socket);
	pthread_mutexattr_setpshared(&matr_client_socket, PTHREAD_PROCESS_PRIVATE);
	pthread_mutex_init(&mutex_client_socket, &matr_client_socket);
	
	if (pthread_create(&client_thread, NULL, &thread_start, this))
	{
		printf("Error creating client thread\n");
		state = STATE_FINISHED;
		return;
	}
	state = STATE_WORKING;
}

std::string Client_connection::get_socket_str(Client_connection *instance)
{
	char txtbuf[BUFFER_LEN];
	inet_ntop(AF_INET, &instance->addr.sin_addr, txtbuf, sizeof(txtbuf));
	
	return std::string(txtbuf) + ":" + std::to_string(instance->addr.sin_port);
}

// READ N BYTES FROM FILE DESCRIPTOR INTO CHAR BUFFER
int custom_read(int socket_fd, char *buf, const int buf_len)
{
	int sum = 0;
	int n;
	while (sum < buf_len)
	{
		n = read(socket_fd, buf+sum, buf_len-sum);
		if (n <= 0)
		{
			return 0;
		}
		sum += n;
	}
	return sum;
}

// THREAD TO WAIT FOR INCOMING COMMANDS
void* Client_connection::thread_start(void *inst)
{
	printf("Client listening thread start\n");
	
	auto *instance = static_cast<Client_connection*>(inst);
	
	char buffer[BUFFER_LEN];
	int *intvec = (int*)buffer;
	bzero(buffer, BUFFER_LEN);
	
	std::cout << "Client connected: " << get_socket_str(instance) << std::endl;
	
	bzero(buffer, BUFFER_LEN);
	int read_status = 1;
	int write_status = 1;
	
	std::cout << "CLient status: " << (instance->status == CSTATUS_ADMIN ? "ADMIN" : "USER") << std::endl;
	
	// Listen for requests
	while (
		strcmp(buffer, "q\r\n") && 
		strcmp(buffer, "q")     &&
		read_status > 0         &&
		instance->state == STATE_WORKING &&
		write_status > 0
	)
	{
		bzero(buffer, BUFFER_LEN);
		
		// WAITING HERE
		pthread_mutex_lock(&instance->mutex_client_socket);
		read_status = custom_read(
			instance->socket_fd, 
			buffer, 
			BUFFER_LEN
		);
		pthread_mutex_unlock(&instance->mutex_client_socket);
		
		
		if (instance->state != STATE_WORKING) break;
		
		auto tokens = tokenize_string(buffer); // Args vector
		
		if (false) {}
		else
		{
			instance->server->command(instance->id, tokens, buffer, BUFFER_LEN);
			if (intvec[0] == MTYPE_DISCONNECT) break;
		}
		
		//printf("\033[1;36mClient \"%s\" message: [%s]\033[0m\n", instance->id.c_str(), buffer);
		
		pthread_mutex_lock(&instance->mutex_client_socket);
		write_status = write(
			instance->socket_fd,
			buffer,
			BUFFER_LEN
		);
		pthread_mutex_unlock(&instance->mutex_client_socket);
		if (write_status < 0) { printf("Error writing data\n"); break; }
	}
	
	instance->state = STATE_FINISHED;
	printf("Client thread finished work\n");
	
	return NULL;
}

Client_connection::~Client_connection(void)
{
	//printf("\033[1;31mClient \"%s\" destructor\033[0m\n", id.c_str());
	
	// shutdown
	if (shutdown(socket_fd, SHUT_RDWR))
		printf("TCP connection shutdown failed\n");
	// close
	if (close(socket_fd))
		printf("Socket closing failed\n");
	state = STATE_FINISHED;
	
	// Try to join client thread
	if (pthread_join(client_thread, NULL))
	{
		printf("Error joining thread\n");
	}
	else
	{
		printf("Stopped client thread\n");
	}
	
	state = STATE_FINISHED;
}

void Client_connection::send(const std::string &message)
{
	char txtbuf[BUFFER_LEN];
	bzero(txtbuf, BUFFER_LEN);
	((int*)txtbuf)[0] = MTYPE_TEXT;
	sprintf(txtbuf + sizeof(int), "%s", message.c_str());
	pthread_mutex_lock(&mutex_client_socket);
	int write_status = write(
		socket_fd,
		txtbuf,
		BUFFER_LEN
	);
	pthread_mutex_unlock(&mutex_client_socket);
}
