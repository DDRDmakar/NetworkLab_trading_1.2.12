
#ifndef _H_SERVER
#define _H_SERVER

#include <vector>
#include <list>
#include <unordered_map>
#include <pthread.h>

#include "client_connection.hpp"
#include "exception.hpp"

struct Lot
{
	int start_price;
	std::list<int> price;
	std::list<std::string> winner;
};

class Server
{
private:
	int port; // Port number
	int maxclients; // Maximum number of clients
	int socket_fd; // Server socket handler
	pthread_t cli_thread, accepting_thread;
	unsigned int current_id;
	bool trading_active; // Is trading still active or not
	
	static void*       cli_thread_start(void *inst);
	static void* accepting_thread_start(void *inst);
	
	std::unordered_map<std::string, Client_connection*> connections;
	std::unordered_map<std::string, Lot> lots;
	
public:
	Server(const int port, const int maxclients);
	~Server(void);
	
	std::string get_lot_list(void);
	void add_lot(const std::string &name, const Lot &newlot);
	Lot& get_lot(const std::string &name);
	void finish(void);
	unsigned int gen_id(void);
	void clear_inactive(void);
	void disconnect(const std::string &id);
	bool is_trading_active(void);
	void command(const std::string &client_id, const std::vector<std::string> &tokens, char *o_buffer, const size_t o_buffer_len);
};

#endif
