
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
	current_id (0),
	trading_active (true)
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
	std::string answer = "LOT LIST\nName    Start price    Price    Winner\n";
	std::string newlotstr;
	for (const auto &e : lots)
	{
		newlotstr = e.first + " " + std::to_string(e.second.start_price) + " " + std::to_string(e.second.price.back()) + " " + e.second.winner.back() + "\n";
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
	
	trading_active = false;
	
	for (auto &e : lots)
	{
		// Get rid of disconnected winners
		while (!e.second.winner.back().empty() && connections.find(e.second.winner.back()) == connections.end())
		{
			e.second.winner.pop_back();
			e.second.price.pop_back();
		}
	}
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

bool Server::is_trading_active(void)
{
	return trading_active;
}


void Server::command(const std::string &client_id, const std::vector<std::string> &tokens, char *o_buffer, const size_t o_buffer_len)
{
	int *intvec = (int*)o_buffer;
	
	if (tokens.size() == 1 && tokens[0] == "list")
	{
		std::string l = get_lot_list();
		sprintf(o_buffer, "%s", l.c_str());
	}
	else if (tokens.size() == 1 && tokens[0] == "q")
	{
		intvec[0] = MTYPE_DISCONNECT;
	}
	else if (tokens.size() == 3 && tokens[0] == "add")
	{
		try
		{
			int start_price = std::stoi(tokens[2]);
			Lot newlot = {start_price, {start_price}, {""}};
			add_lot(tokens[1], newlot);
		}
		catch (Exception &e) // Incorrect client ID
		{
			intvec[0] = MTYPE_ERROR_NAME;
		}
		catch (std::exception &e) // Incorrect number passed
		{
			intvec[0] = MTYPE_ERROR_NUM;
		}
	}
	else if (tokens.size() == 3 && tokens[0] == "price")
	{
		int new_price = std::stoi(tokens[2]);
		std::string lot_name = tokens[1];
		try
		{
			// Get reference to lot
			Lot &current_lot = get_lot(lot_name);
			
			if (
				(current_lot.price.back() < new_price) || 
				(current_lot.price.back() <= new_price && current_lot.winner.back().empty())
			)
			{
				current_lot.price.push_back(new_price);
				current_lot.winner.push_back(client_id);
			}
			else printf("Error - price %d is not the highest\n", new_price);
		}
		catch (Exception &e)
		{
			intvec[0] = MTYPE_ERROR_NAME;
		}
	}
	else if (tokens.size() == 1 && tokens[0] == "finish")
	{
		printf("Finish trading\n");
		finish();
	}
	else if (tokens.size() == 2 && tokens[0] == "disconnect")
	{
		if (tokens[1] == client_id) intvec[0] = MTYPE_ERROR_DISCONNECT;
		else disconnect(tokens[1]);
	}
	else
	{
		printf("Error - wrong message type\n");
	}
}
