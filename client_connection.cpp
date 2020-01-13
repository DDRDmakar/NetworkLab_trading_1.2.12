
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


Client_connection::Client_connection(Server *server, const int socket_fd, const sockaddr_in addr, const CLIENT_STATUS status, const std::string id) :
	server (server),
	socket_fd (socket_fd),
	addr (addr),
	status (status),
	id (id)
{
	printf("\033[1;32mClient \"%s\" constructor\033[0m\n", id.c_str());
	
	// Recive buffer with authorization info
	
	
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
	char buffer[BUFFER_LEN];
	inet_ntop(AF_INET, &instance->addr.sin_addr, buffer, sizeof(buffer));
	
	return "Client connected: " + std::string(buffer) + ":" + std::to_string(instance->addr.sin_port);
}

// READ N BYTES FROM FILE DESCRIPTOR INTO CHAR BUFFER
int custom_read(int socket_fd, char *buf, const int buf_len)
{
	int sum = 0;
	int n;
	while (sum < buf_len)
	{
		n = read(socket_fd, buf+sum, buf_len-sum);
		if (n <= 0) return 0;
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
		read_status = custom_read(
			instance->socket_fd, 
			buffer, 
			BUFFER_LEN
		);
		
		if (instance->state != STATE_WORKING) break;
		
		int mtype = intvec[0];
		switch (mtype)
		{
			case MTYPE_LIST:
			{
				printf("Send list to client\n");
				std::string l = instance->server->get_lot_list();
				sprintf(buffer, "%s", l.c_str());
				break;
			}
			case MTYPE_ADD:
			{
				printf("Add lot\n");
				int start_price = intvec[1];
				char *lot_name = buffer+(2*sizeof(int));
				Lot newlot = {start_price, start_price, ""};
				try
				{
					instance->server->add_lot(lot_name, newlot);
				}
				catch (Exception &e)
				{
					intvec[0] = MTYPE_ERROR_NAME;
				}
				break;
			}
			case MTYPE_PRICE:
			{
				printf("Change lot price\n");
				int new_price = intvec[1];
				char *lot_name = buffer+(2*sizeof(int));
printf("lot name \"%s\", price %d ptr %p %p\n", lot_name, new_price, instance, instance->server);
				try
				{
					Lot &current_lot = instance->server->get_lot(lot_name);
					if ((current_lot.price < new_price) || (current_lot.winner.empty() && current_lot.price <= new_price))
					{
						current_lot.price = new_price;
					}
					else printf("Error - price %d is not the highest\n", new_price);
				}
				catch (Exception &e)
				{
					intvec[0] = MTYPE_ERROR_NAME;
				}
				break;
			}
			case MTYPE_FINISH:
			{
				printf("Finish trading\n");
				instance->server->finish();
				//read_status = 0;
				//continue;
				break;
			}
			default:
			{
				printf("Error - wrong message type\n");
				break;
			}
		}
		
		printf("\033[1;36mClient \"%s\" message: [%s]\033[0m\n", instance->id.c_str(), buffer);
		
		write_status = write(
			instance->socket_fd,
			buffer,
			BUFFER_LEN
		);
		if (write_status < 0) { printf("Error writing data\n"); break; }
	}
	
	instance->state = STATE_FINISHED;
	printf("Client thread finished work\n");
	
	return NULL;
}

Client_connection::~Client_connection(void)
{
	printf("\033[1;31mClient \"%s\" destructor\033[0m\n", id.c_str());
	
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
