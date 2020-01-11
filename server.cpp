
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
	maxclients (maxclients)
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
		
		/////////////////TODO join_threads();
		
		if (instance->connections.size() >= instance->maxclients)
		{
			std::cout << "Unable to create more client threads\n";
			continue;
		}
		
		// Construct new connection at vector back
		instance->connections.emplace_back(instance, client_handler, client_addr);
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
	lots.insert(std::make_pair(name, newlot));
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
