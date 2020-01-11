
#ifndef _H_CLIENT_CONNECTION
#define _H_CLIENT_CONNECTION

#include <string>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <arpa/inet.h>
//#include <sys/stat.h>

class Server;

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

enum CLIENT_STATUS
{
	CSTATUS_USER,
	CSTATUS_ADMIN
};

class Client_connection
{
private:
	enum STATE
	{
		STATE_WORKING,  // Thread is active - listening client messages
		STATE_FINISHED  // CLient disconnected
	};
	
	Server *server;
	pthread_t client_thread;
	
	static void* thread_start(void *inst);
	
public:
	STATE state;
	const int socket_fd;
	sockaddr_in addr;
	CLIENT_STATUS status;
	
	
	Client_connection(Server *server, const int socket_fd, const sockaddr_in addr, const CLIENT_STATUS status);
	static std::string get_socket_str(Client_connection *instance);
};

#endif
