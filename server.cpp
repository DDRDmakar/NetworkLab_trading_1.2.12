
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
	
	pthread_mutexattr_init(&matr_connections);
	pthread_mutexattr_init(&matr_lots);
	pthread_mutexattr_setpshared(&matr_connections, PTHREAD_PROCESS_PRIVATE);
	pthread_mutexattr_setpshared(&matr_lots,        PTHREAD_PROCESS_PRIVATE);
	pthread_mutex_init(&mutex_connections, &matr_connections);
	pthread_mutex_init(&mutex_lots,        &matr_lots);
	
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
	
	while (instance->is_trading_active())
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
		if (client_handler < 0 || !instance->is_trading_active()) break; // If server socket is closed
		
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
		pthread_mutex_lock(&instance->mutex_connections);
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
		pthread_mutex_unlock(&instance->mutex_connections);
	}
	
	return NULL;
}

// Server class destructor. Closing connections, freeing memory
Server::~Server(void)
{
	//std::cout << "Server destructor\n" << std::endl;
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
	pthread_mutex_lock(&mutex_lots);
	std::string answer = "LOT LIST\nName    Start price    Price    Winner\n";
	std::string newlotstr;
	for (const auto &e : lots)
	{
		newlotstr = e.first + " " + std::to_string(e.second.start_price) + " " + std::to_string(e.second.price.back()) + " " + e.second.winner.back() + "\n";
		answer += newlotstr;
	}
	pthread_mutex_unlock(&mutex_lots);
	return answer;
}

std::string Server::get_user_list(void)
{
	pthread_mutex_lock(&mutex_connections);
	std::string answer = "USERS LIST\nName    Socket\n";
	std::string newuserstr;
	for (const auto &e : connections)
	{
		if (e.second->state == STATE_WORKING)
		{
			newuserstr = e.first + " " + e.second->get_socket_str(e.second) + "\n";
			answer += newuserstr;
		}
	}
	pthread_mutex_unlock(&mutex_connections);
	return answer;
}

void Server::add_lot(const std::string &name, const Lot &newlot)
{
	pthread_mutex_lock(&mutex_lots);
	auto res = lots.find(name);
	if (res == lots.end()) lots.insert(std::make_pair(name, newlot));
	else
	{
		printf("Error adding lot with the same name\n");
	}
	pthread_mutex_unlock(&mutex_lots);
}

void Server::finish(void)
{
	trading_active = false;
	
	pthread_mutex_lock(&mutex_connections);
	pthread_mutex_lock(&mutex_lots);
	// Get rid of disconnected winners
	for (auto &e : lots)
	{
		while (!e.second.winner.back().empty() && connections.find(e.second.winner.back()) == connections.end())
		{
			e.second.winner.pop_back();
			e.second.price.pop_back();
		}
	}
	
	pthread_mutex_unlock(&mutex_connections);
	pthread_mutex_unlock(&mutex_lots);
	
	clear_inactive();
	
	// Send results to all clients
	std::string trading_result = get_lot_list();
	for (const auto &e : connections)
	{
		e.second->send(trading_result);
	}
	
}

unsigned int Server::gen_id(void)
{
	return ++current_id;
}

void Server::clear_inactive(void)
{
	pthread_mutex_lock(&mutex_connections);
	for (auto it = connections.begin(); it != connections.end(); it++)
	{
		if (it->second->state == STATE_FINISHED)
		{
			delete it->second;
			connections.erase(it);
			break;
		}
	}
	pthread_mutex_unlock(&mutex_connections);
}

void Server::disconnect(const std::string &id)
{
	pthread_mutex_lock(&mutex_connections);
	auto res = connections.find(id);
	if (res != connections.end())
	{
		delete res->second;
		connections.erase(res);
	}
	pthread_mutex_unlock(&mutex_connections);
}

bool Server::is_trading_active(void)
{
	return trading_active;
}

// Execute console command (from client console or from server console)
void Server::command(const std::string &client_id, const std::vector<std::string> &tokens, char *o_buffer, const size_t o_buffer_len)
{
	
	pthread_mutex_lock(&mutex_connections);
	auto res_status = connections.find(client_id);
	if (!client_id.empty() && res_status == connections.end())
	{
		pthread_mutex_unlock(&mutex_connections);
		return;
	}
	// If id is empty string, we work from server console
	CLIENT_STATUS client_status = client_id.empty() ? CSTATUS_ADMIN : res_status->second->status;
	pthread_mutex_unlock(&mutex_connections);
	
	int *intvec = (int*)o_buffer;
	
	if (tokens.size() == 2 && tokens[0] == "list")
	{
		intvec[0] = MTYPE_TEXT;
		if (tokens[1] == "lots")
		{
			std::string l = get_lot_list();
			sprintf(o_buffer + sizeof(int), "%s", l.c_str());
		}
		else if (tokens[1] == "users")
		{
			
			std::string l = get_user_list();
			sprintf(o_buffer + sizeof(int), "%s", l.c_str());
		}
		else
		{
			intvec[0] = MTYPE_ERROR_COMMAND;
		}
	}
	else if (tokens.size() == 1 && tokens[0] == "q")
	{
		trading_active = false;
		// DIsconnect all clients
		for (const auto &e : connections)
		{
			disconnect(e.second->id);
		}
		intvec[0] = MTYPE_DISCONNECT;
	}
	else if (tokens.size() == 3 && tokens[0] == "add")
	{
		try
		{
			if (client_status == CSTATUS_ADMIN)
			{
				int start_price = std::stoi(tokens[2]);
				Lot newlot = {start_price, {start_price}, {""}};
				add_lot(tokens[1], newlot);
				intvec[0] = MTYPE_RESPONSE_OK;
			}
			else
			{
				intvec[0] = MTYPE_ERROR_RIGHTS;
			}
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
		
		pthread_mutex_lock(&mutex_lots);
		auto res = lots.find(lot_name);
		if (res != lots.end()) 
		{
			Lot &current_lot = res->second;
			if (
				(current_lot.price.back() < new_price) || 
				(current_lot.price.back() <= new_price && current_lot.winner.back().empty())
			)
			{
				current_lot.price.push_back(new_price);
				current_lot.winner.push_back(client_id);
				intvec[0] = MTYPE_RESPONSE_OK;
			}
			else
			{
				intvec[0] = MTYPE_ERROR_PRICE;
				printf("Error - price %d is not the highest\n", new_price);
			}
		}
		else
		{
			intvec[0] = MTYPE_ERROR_NAME;
		}
		pthread_mutex_unlock(&mutex_lots);
	}
	else if (tokens.size() == 1 && tokens[0] == "finish")
	{
		if (client_status == CSTATUS_ADMIN)
		{
			printf("Finish trading\n");
			finish();
			intvec[0] = MTYPE_RESPONSE_OK;
		}
		else intvec[0] = MTYPE_ERROR_RIGHTS;
	}
	else if (tokens.size() == 2 && tokens[0] == "disconnect")
	{
		if (client_status == CSTATUS_ADMIN)
		{
			if (tokens[1] == client_id) intvec[0] = MTYPE_ERROR_DISCONNECT;
			else
			{
				disconnect(tokens[1]);
				intvec[0] = MTYPE_RESPONSE_OK;
			}
		}
		else intvec[0] = MTYPE_ERROR_RIGHTS;
	}
	else
	{
		printf("Error - wrong message type\n");
		intvec[0] = MTYPE_ERROR_COMMAND;
	}
}
