
#ifndef _H_SERVER
#define _H_SERVER

#include <vector>
#include <pthread.h>

#include "client_connection.hpp"
#include "exception.hpp"

struct Lot
{
	std::string name;
	int price, start_price;
};

class Server
{
private:
	int port; // Port number
	int maxclients; // Maximum number of clients
	int socket_fd; // Server socket handler
	pthread_t cli_thread, accepting_thread;
	
	static void*       cli_thread_start(void *inst);
	static void* accepting_thread_start(void *inst);
	
	std::vector<Client_connection> connections;
	std::vector<Lot> lots;
	
public:
	Server(const int port, const int maxclients);
	~Server(void);
	
	std::string get_lot_list(void);
	void add_lot(Lot &newlot);
	void finish(void);
};

#endif
