
#include <string.h>
#include <iostream>
#include <unistd.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/stat.h>

#include "server.hpp"

// Server class constructor. Initializing listening port
Server::Server(const int port, const int maxclients) : 
	port (port),
	maxclients (maxclients),
	current_id (0)
{
	// Create socket
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (socket_fd < 0) throw Exception("Socket creation failed");
	
	// Initialize server address
	struct sockaddr_in serv_addr;
	bzero((char *) &serv_addr, sizeof(serv_addr));
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = INADDR_ANY;
	serv_addr.sin_port = htons(port);
	
	printf("Server socket: localhost:%d\n", port);
	
	// Bind socket
	int bind_status = bind(
		socket_fd,
		(struct sockaddr *) &serv_addr,
		sizeof(serv_addr)
	);
	if (bind_status) throw Exception("Bind socket failed");
	
	listen(socket_fd, 5);
	
	// Creating new thread to accept new connections
	if (pthread_create(&accepting_thread, NULL, accepting_thread_start, this))
	{
		throw Exception("Error creating client-accepting thread");
	}
}

// THREAD TO WAIT FOR NEW CONNECTIONS
void* Server::accepting_thread_start(void *inst)
{
	std::cout << "Accepting thread started" << std::endl;
	Server *instance = static_cast<Server*>(inst);
	
	char buffer[BUFFER_LEN];
	int *intvec = (int*)buffer;
	
	while (true)
	{
		listen(instance->socket_fd, 5);
		
		struct sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		// WAITING HERE
		int client_handler = accept(
			instance->socket_fd,
			(struct sockaddr *)&client_addr,
			&client_addr_len
		);
		if (client_handler < 0) break; // If server socket is closed
		
		if (instance->connections.size() >= instance->maxclients)
		{
			std::cout << "Unable to create more client threads\n";
			continue;
		}
		
		instance->clear_inactive();
		
		// Authorization
		// Here we wait for 4 bytes of data - admin access key
		// And following string - user ID
		bzero(buffer, BUFFER_LEN);
		int read_status = custom_read(
			client_handler, 
			buffer, 
			BUFFER_LEN
		);
		printf("Read status %d\n", read_status);
		CLIENT_STATUS client_status;
		if (intvec[0] == ADMIN_KEY) client_status = CSTATUS_ADMIN;
		else client_status = CSTATUS_USER;
		char *client_id = buffer + sizeof(int);
		
		printf("User ID is %s\n", client_id);
		// Construct new connection
		auto res = instance->connections.find(client_id);
		auto newconn = new Client_connection(instance, client_handler, client_addr, client_status, client_id);
		if (res == instance->connections.end())
		{
			instance->connections.insert(
				std::make_pair(client_id, newconn)
			);
		}
		else
		{
			printf("Error adding client with the same name\n");
			delete newconn;
		}
	}
	
	return NULL;
}

// Server class destructor. Closing connections, freeing memory
Server::~Server(void)
{
	std::cout << "Server destructor\n" << std::endl;
	if (socket_fd >= 0)
	{
		int error_code;
		unsigned int error_code_size = sizeof(error_code);
		getsockopt(socket_fd, SOL_SOCKET, SO_ERROR, &error_code, &error_code_size);
		if (error_code == 0)
		{
			if (shutdown(socket_fd, SHUT_RDWR)) printf("Server connection shutdown failed\n");
			if (close(socket_fd)) printf("Server socket closing failed\n");
		}
	}
}

std::string Server::get_lot_list(void)
{
	std::string answer = "LOT LIST\nName    Start price    Price\n";
	std::string newlotstr;
	for (const auto &e : lots)
	{
		newlotstr = e.first + " " + std::to_string(e.second.start_price) + " " + std::to_string(e.second.price) + "\n";
		answer += newlotstr;
	}
	return answer;
}

void Server::add_lot(const std::string &name, const Lot &newlot)
{
	auto res = lots.find(name);
	if (res == lots.end()) lots.insert(std::make_pair(name, newlot));
	else printf("Error adding lot with the same name\n");
}

void Server::finish(void)
{
	printf("Server: finish all lots");
}

Lot& Server::get_lot(const std::string &name)
{
	auto res = lots.find(name);
	if (res == lots.end()) throw Exception("Invalid lot name");
	else return res->second;
}

unsigned int Server::gen_id(void)
{
	return ++current_id;
}

void Server::clear_inactive(void)
{
	for (auto it = connections.begin(); it != connections.end(); it++)
	{
		if (it->second->state == STATE_FINISHED)
		{
			delete it->second;
			connections.erase(it);
			break;
		}
	}
}

void Server::disconnect(const std::string &id)
{
	auto res = connections.find(id);
	if (res != connections.end())
	{
		delete res->second;
		connections.erase(res);
	}
}
